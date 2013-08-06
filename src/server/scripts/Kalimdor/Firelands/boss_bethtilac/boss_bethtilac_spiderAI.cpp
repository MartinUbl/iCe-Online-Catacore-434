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

/*
 * implementation of common ancestor class for creatures in Beth'tilac encounter
 */


#include "ScriptPCH.h"
#include "boss_bethtilac_spiderAI.h"
#include "boss_bethtilac_data.h"

#include <list>
using namespace std;

enum CommonEvents
{
    CHECK_CONSUME = -1
};


SpiderAI::SpiderAI(Creature *creature)
    : ScriptedAI(creature)
{
    instance = creature->GetInstanceScript();

    summonFilament = NULL;
}


SpiderAI::~SpiderAI()
{
    //UnSummonFilament();   // not a valid pointer, was deleted silently
}


void SpiderAI::AddTimer(int32 action, uint32 delay, bool repeat)
{
    Timer event;
    event.action = action;
    event.repeat = repeat;
    event.timer = delay;
    event.totalTime = delay;

    if (delay == 0)
        timers.push_back(event);    // process in the current update
    else
        timers.push_front(event);   // timer starts in the next update
}

/*
void spiderAI::RemoveTimer(int32 timerId)
{
    for (TimerList::iterator it = timers.begin(); it != timers.end(); it++)
    {
        if (it->action == timerId)
        {
            timers.erase(it);
            break;
        }
    }
}
*/

void SpiderAI::ClearTimers()
{
    timers.clear();
}


void SpiderAI::UpdateTimers(const uint32 diff)
{
    for (TimerList::iterator it = timers.begin(); it != timers.end(); )
    {
        if ((it->timer -= diff) < 0)
        {
            DoAction(it->action);

            if (!it->repeat)
                it = timers.erase(it);
            else
            {
                it->timer += it->totalTime;
                it++;
            }
        }
        else
            it++;
    }
}


void SpiderAI::DebugOutput(const char *str) const
{
#ifdef _DEBUG
    //me->MonsterTextEmote(str, 0, true);
    me->MonsterYell(str, LANG_UNIVERSAL, 0);
#endif
}


uint32 SpiderAI::GetPower() const
{
    return me->GetPower(POWER_MANA);
}


void SpiderAI::SetMaxPower(uint32 newVal)
{
    me->SetMaxPower(POWER_MANA, newVal);
}


void SpiderAI::SetPower(uint32 newVal)
{
    me->SetPower(POWER_MANA, newVal);
}


void SpiderAI::ModifyPower(int32 diff)
{
    me->ModifyPower(POWER_MANA, diff);
}


void SpiderAI::SummonFilament()
{
    if (summonFilament)
        UnSummonFilament();

    Position pos;
    me->GetPosition(&pos);
    pos.m_positionZ = webZPosition + 1.0f;
    if ((summonFilament = me->SummonCreature(NPC_FILAMENT_CASTER, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 0, TEMPSUMMON_MANUAL_DESPAWN, 0)))
    {
        summonFilament->StopMoving();
        summonFilament->SetReactState(REACT_PASSIVE);
        summonFilament->SetFlying(true);
        summonFilament->CastSpell(me, SPELL_SPIDERWEB_FILAMENT, false);

        summonFilament->SendMovementFlagUpdate();
    }
}


void SpiderAI::UnSummonFilament()
{
    if (summonFilament)
        summonFilament->DespawnOrUnsummon();

    summonFilament = NULL;
}

/*
Position SpiderAI::GetFilamentPosition() const
{
    Position pos;

    if (summonFilament)
        summonFilament->GetPosition(&pos);
    else
        me->GetPosition(&pos);

    return pos;
}
*/

void SpiderAI::MoveToGround(uint32 movementId)
{
    me->AttackStop();
    me->DeleteThreatList();
    me->SetFlying(true);
    me->SetReactState(REACT_PASSIVE);

    float posX = me->GetPositionX(),
          posY = me->GetPositionY(),
          posZ = me->GetPositionZ();

    posZ = me->GetMap()->GetHeight(posX, posY, posZ - 10.0f);  // height of the closest ground
    //posZ += me->GetObjectSize();                               // do not dig in to the ground

    me->GetMotionMaster()->MovePoint(movementId, posX, posY, posZ);
}


void SpiderAI::MoveToFilament(uint32 movementId)
{
    if (summonFilament)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlying(true);

        float posX = summonFilament->GetPositionX(),
              posY = summonFilament->GetPositionY(),
              posZ = summonFilament->GetPositionZ();

        me->GetMotionMaster()->MovePoint(movementId, posX, posY, posZ);
    }
}


void SpiderAI::SummonedCreatureDespawn(Creature *creature)
{
    if (creature == summonFilament)
        summonFilament = NULL;
    ScriptedAI::SummonedCreatureDespawn(creature);
}


void SpiderAI::JustDied(Unit *killer)
{
    UnSummonFilament();
    ScriptedAI::JustDied(killer);
}


void SpiderAI::MoveInLineOfSight(Unit *who)
{
    if (me->IsVehicle() && me->GetEntry() != NPC_SPIDERWEB_FILAMENT &&
        !me->IsNonMeleeSpellCasted(false))        // can eat spiderlings
    {
        if (Creature *creature = who->ToCreature())
        {
            if (creature->GetEntry() == NPC_CINDERWEB_SPIDERLING)
            {
                if (creature->GetExactDist(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()
                    < GetSpellMaxRange(SPELL_CONSUME, false)))
                {
                    me->CastSpell(creature, SPELL_CONSUME, false);
                    AddTimer(CHECK_CONSUME, 1200, false);   // check for another spiderling in range after eating this one
                    return;     // the spell is channeled - can not attack until finished
                }
            }
        }
    }

    ScriptedAI::MoveInLineOfSight(who);
}


void SpiderAI::SpellHit(Unit *victim, SpellEntry const *spell)
{
    Creature *creature = victim->ToCreature();

    if (spell->Id == SPELL_CONSUME && creature)
    {
        // find another victim to eat
        if (Unit *nextVictim = me->FindNearestCreature(creature->GetEntry(), GetSpellMaxRange(spell, IsPositiveSpell(spell->Id))))
        {
            MoveInLineOfSight(nextVictim);
        }
    }
}


void SpiderAI::DoAction(const int32 event)
{
    switch (event)
    {
        case CHECK_CONSUME:
        {
            // hack-fix to make the spider consume another spiderling after eating one
            float range = GetSpellMaxRange(SPELL_CONSUME, false);
            Unit *spiderling = me->FindNearestCreature(NPC_CINDERWEB_SPIDERLING, range, true);
            if (spiderling)
                MoveInLineOfSight(spiderling);
        }
            break;
        default:
            break;
    }
}
