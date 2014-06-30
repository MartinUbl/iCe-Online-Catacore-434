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
 * implementation of AI for summon Cinderweb Drone
 * as part of the script of Beth'tilac
 */


#include "ScriptPCH.h"
#include "../firelands.h"
#include "boss_bethtilac_data.h"
#include "boss_bethtilac_spiderAI.h"


namespace Bethtilac
{


class mob_droneAI: public SpiderAI
{
public:
    explicit mob_droneAI(Creature *creature);
    virtual ~mob_droneAI();

private:
    // virtual method overrides
    void Reset();
    void JustDied(Unit *killer);
    void EnterCombat(Unit *who);
    void EnterEvadeMode();
    void UpdateAI(const uint32 diff);
    void DoAction(const int32 event);
    void MovementInform(uint32 type, uint32 id);
    void AttackStart(Unit *victim);
    void IsSummonedBy(Unit *summoner);
    Player *SelectPlayerOnGround(void);

    // attributes
    bool onGround;
    bool onTop;
    uint32 fixateTimer;
};


class mob_drone: public CreatureScript
{
public:
    mob_drone(): CreatureScript("mob_cinderweb_drone") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        if (creature->isSummon())
            return new mob_droneAI(creature);

        return NULL;
    }
};

void load_npc_CinderwebDrone()
{
    new mob_drone();
};





//////////////////////////////////////////////////////////////////////////
// implementation of Cinderweb Drone

enum DroneSpells
{
    SPELL_LEECH_VENOM = 99411,
    SPELL_BOILING_SPATTER = 99463,
    SPELL_DRONE_BURNING_ACID = 99934,
    SPELL_FIXATE = 99559,
};

enum DroneEvents
{
    POWER_DEPLETE = 0,
    END_OF_LEECH_VENOM_CAST,
    CAST_BOILING_SPATTER,
    CAST_BURNING_ACID,
};



mob_droneAI::mob_droneAI(Creature *creature)
    : SpiderAI(creature)
    , onGround(true)
    , onTop(false)
{
}


mob_droneAI::~mob_droneAI()
{
}


void mob_droneAI::Reset()
{
    SpiderAI::Reset();
    fixateTimer = 7000;
}


void mob_droneAI::EnterCombat(Unit *who)
{
    SpiderAI::EnterCombat(who);
}

void mob_droneAI::JustDied(Unit *killer)
{
    if (instance && (me->GetPositionZ() + 5.0f) < webZPosition) // 5 yard tolerance for sure
        instance->SetData(DATA_DEATH_FROM_ABOVE_ACHIEV, 0); // Failed achiev
}

void mob_droneAI::EnterEvadeMode()
{
    // do nothing
}

Player* mob_droneAI::SelectPlayerOnGround()
{
    if (!instance)
        return NULL;

    Map::PlayerList const& plList = instance->instance->GetPlayers();

    if (plList.isEmpty())
        return NULL;

    std::list<Player*> groundPlayers;
    groundPlayers.clear();

    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
    {
        if (Player * pl = itr->getSource())
        {
            if (pl->GetPositionZ() < (85.0f))
                groundPlayers.push_back(pl);
        }
    }

    if (groundPlayers.empty())
        return NULL;

    std::list<Player*>::iterator j = groundPlayers.begin();
    advance(j, rand() % groundPlayers.size()); // Pick random target

    if ((*j) && (*j)->IsInWorld())
    {
        return (*j);
    }
    return NULL;
}


void mob_droneAI::UpdateAI(const uint32 diff)
{
    if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
    {
        UnSummonFilament();
        me->DespawnOrUnsummon();
        return;
    }

    if (me->HasAura(SPELL_FIXATE))
    {
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
    }
    else
    {
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
    }

    if (me->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        return;

    if (UpdateVictim())
    {
        if (IsHeroic() && fixateTimer <= diff)
        {
            if (Player * pl = SelectPlayerOnGround())
            {
                pl->CastSpell(me, SPELL_FIXATE, true);
            }
            fixateTimer = urand(25000, 35000);
        }
        else fixateTimer -= diff;

        DoMeleeAttackIfReady();
    }


    UpdateTimers(diff);
}


void mob_droneAI::DoAction(const int32 event)
{
    switch (event)
    {
        case POWER_DEPLETE:
            ModifyPower(-1);

            if (GetPower() == 0)
            {
                // if Beth'tilac is on the web above, follow her and cast Leech Venom
                Unit *summoner = me->ToTempSummon()->GetSummoner();
                if (summoner->GetPositionZ() >= webZPosition)
                {
                    onGround = false;
                    SummonFilament();
                    MoveToFilament(MOVE_POINT_UP);
                }
                else
                    MovementInform(POINT_MOTION_TYPE, MOVE_POINT_UP);
            }
            break;

        case END_OF_LEECH_VENOM_CAST:
            onTop = false;

            if (me->GetPositionZ() >= webZPosition)
            {
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 5.0f, me->GetOrientation());
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                SummonFilament();
                MoveToGround(MOVE_POINT_DOWN);
            }
            else
                MovementInform(POINT_MOTION_TYPE, MOVE_POINT_DOWN);

            break;

        case CAST_BOILING_SPATTER:
            me->CastSpell(me->getVictim(), SPELL_BOILING_SPATTER, false);
            break;

        case CAST_BURNING_ACID:
            me->CastCustomSpell(SPELL_DRONE_BURNING_ACID, SPELLVALUE_MAX_TARGETS, 1, NULL, false);
            break;

        default:
            SpiderAI::DoAction(event);
            break;
    }
}


void mob_droneAI::MovementInform(uint32 type, uint32 id)
{
    if (type == POINT_MOTION_TYPE)
    {
        switch (id)
        {
            case MOVE_POINT_UP:
                onTop = true;
                UnSummonFilament();
                me->PlayOneShotAnimKit(ANIM_KIT_FLY_UP);
                me->SetFlying(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->CastSpell(me->ToTempSummon()->GetSummoner(), SPELL_LEECH_VENOM, false);
                AddTimer(END_OF_LEECH_VENOM_CAST, 1, false);        // immediately after end of channel
                break;

            case MOVE_POINT_DOWN:
                onGround = true;
                UnSummonFilament();
                me->SetFlying(false);
                me->SetReactState(REACT_AGGRESSIVE);
                break;
        }
    }
}


void mob_droneAI::AttackStart(Unit *victim)
{
    if (victim->GetPositionZ() < webZPosition - 10.0f)      // don't attack players above the web
        SpiderAI::AttackStart(victim);
}


void mob_droneAI::IsSummonedBy(Unit *summoner)
{
    SetMaxPower(85);
    SetPower(85);

    AddTimer(POWER_DEPLETE, 1000, true);
    AddTimer(CAST_BOILING_SPATTER, 15000, true);
    AddTimer(CAST_BURNING_ACID, 7000, true);

    Position pos;
    summoner->GetPosition(&pos);
    pos.m_positionZ -= 20.0f;
    pos.m_positionZ = summoner->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); // get ground height
    me->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());    // go underneath Beth'tilac

    DoZoneInCombat();

    onGround = true;
    onTop = false;
}


}   // end of namespace
