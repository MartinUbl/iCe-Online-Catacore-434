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
 * implementation of AI for summon Cinderweb Spiderling
 * as part of the script of Beth'tilac
 */


#include "ScriptPCH.h"
#include "../firelands.h"
#include "boss_bethtilac_data.h"
#include "boss_bethtilac_spiderAI.h"


namespace Bethtilac
{


class mob_spiderlingAI: public SpiderAI
{
public:
    explicit mob_spiderlingAI(Creature *creature);
    virtual ~mob_spiderlingAI();

private:
    // virtual method overrides
    void UpdateAI(const uint32 diff);
    void DoAction(const int32 event);
    void MovementInform(uint32 type, uint32 id);
    void MoveInLineOfSight(Unit *who);
    void IsSummonedBy(Unit *summoner);
    void EnterEvadeMode();

    // attributes
    uint64 followedGuid;
    bool following;

    // methods
    bool CanFollowTarget(Unit *target) const;
    Unit *ChooseTarget();
    bool FollowTarget();
};


class mob_spiderling: public CreatureScript
{
public:
    mob_spiderling(): CreatureScript("mob_cinderweb_spiderling") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        if (creature->IsSummon())
            return new mob_spiderlingAI(creature);

        return NULL;
    }
};

void load_mob_Spiderling()
{
    new mob_spiderling();
}



//////////////////////////////////////////////////////////////////////////
// implementation of Cinderweb Spiderling


static const uint32 MOVE_CHASE = 1;

enum SpiderlingEvents
{
    MOVE_CHECK = 0
};

enum SpiderlingSpells
{
    SPELL_SEEPING_VENOM = 99130
};


mob_spiderlingAI::mob_spiderlingAI(Creature *creature)
    : SpiderAI(creature)
    , followedGuid(0)
    , following(false)
{
}


mob_spiderlingAI::~mob_spiderlingAI()
{
}



void mob_spiderlingAI::EnterEvadeMode()
{
    // do nothing, don't evade
}


void mob_spiderlingAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        me->DespawnOrUnsummon();
        return;
    }

    UpdateTimers(diff);
}


void mob_spiderlingAI::DoAction(const int32 event)
{
    switch (event)
    {
        case MOVE_CHECK:
            FollowTarget();
            break;
        default:
            break;
    }
}


void mob_spiderlingAI::MovementInform(uint32 type, uint32 id)
{
    me->StopMoving();
    FollowTarget();
}


void mob_spiderlingAI::MoveInLineOfSight(Unit *who)
{
    if (!me->HasSpellCooldown(SPELL_SEEPING_VENOM))
    {
        me->CastSpell(me, SPELL_SEEPING_VENOM, false);
        me->AddCreatureSpellCooldown(SPELL_SEEPING_VENOM);
    }
}


void mob_spiderlingAI::IsSummonedBy(Unit *summoner)
{
    FollowTarget();

    // re-check after a second
    AddTimer(MOVE_CHECK, 1000, true);
}


Unit *mob_spiderlingAI::ChooseTarget()
{
    // find Drone or Beth'tilac to follow to be eaten
    if (Creature *drone = me->FindNearestCreature(NPC_CINDERWEB_DRONE, 100.0f, true))
        if (CanFollowTarget(drone))
            return drone;

    if (Creature *beth = me->FindNearestCreature(NPC_BETHTILAC, 100.0f, true))
    {
        if (CanFollowTarget(beth))
            return beth;
    }

    return NULL;
}


bool mob_spiderlingAI::FollowTarget()
{
    if (following)
    {
        if (Unit *followed = ObjectAccessor::GetUnit(*me, followedGuid))
        {
            if (CanFollowTarget(followed))
            {
                me->GetMotionMaster()->MoveChase(followed);
                return true;
            }
        }
    }

    if (Unit *unit = ChooseTarget())
    {
        following = true;
        followedGuid = unit->GetGUID();
        me->GetMotionMaster()->MoveChase(unit);

        return true;
    }

    // move to the middle and wait
    //DoZoneInCombat();
    Position pos;
    if (Unit *summoner = me->ToTempSummon()->GetSummoner())
    {
        summoner->GetPosition(&pos);
        pos.m_positionZ -= 20.0f;
        pos.m_positionZ = summoner->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); // get ground height
        me->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());    // go underneath Beth'tilac
    }

    following = false;
    followedGuid = 0;

    return false;
}


bool mob_spiderlingAI::CanFollowTarget(Unit *target) const
{
    return target->IsAlive() && target->GetPositionZ() < webZPosition - 20.0f;
}


}   // end of namespace
