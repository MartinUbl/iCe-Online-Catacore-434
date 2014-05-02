/*
 * Copyright (C) 2005-2011 MaNGOS <http://www.getmangos.com/>
 *
 * Copyright (C) 2008-2011 Trinity <http://www.trinitycore.org/>
 *
 * Copyright (C) 2010-2011 Project SkyFire <http://www.projectskyfire.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "gamePCH.h"
#include "TotemAI.h"
#include "Totem.h"
#include "Creature.h"
#include "DBCStores.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"
#include "Spell.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

int TotemAI::Permissible(const Creature *creature)
{
    if (creature->isTotem())
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

TotemAI::TotemAI(Creature *c) : CreatureAI(c), i_victimGuid(0), i_targetSearchTimer(0)
{
    ASSERT(c->isTotem());

    // Search spell
    SpellEntry const *spellInfo = sSpellStore.LookupEntry(me->ToTotem()->GetSpell());
    if (!spellInfo)
        return;

    // Get spell range
    SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(spellInfo->rangeIndex);
    m_maxRange = GetSpellMaxRangeForHostile(srange);

    // SPELLMOD_RANGE not applied in this place just because range mods doesn't exist for attacking totems
}

void TotemAI::MoveInLineOfSight(Unit *)
{
}

void TotemAI::EnterEvadeMode()
{
    me->CombatStop(true);
}

void TotemAI::UpdateAI(const uint32 diff)
{
    if (me->ToTotem()->GetTotemType() != TOTEM_ACTIVE)
        return;

    if (!me->isAlive())
        return;

    // pointer to appropriate target if found any
    Unit* victim = i_victimGuid ? ObjectAccessor::GetUnit(*me, i_victimGuid) : NULL;

    // Search victim if no, not attackable, or out of range, or friendly (possible in case duel end)
    if (!CheckCurrentTarget(victim))
        victim = UpdateTarget();

    if (i_targetSearchTimer <= diff)
    {
        victim = UpdateTarget();
        // the timer is set inside UpdateTarget routine
    }
    else
        i_targetSearchTimer -= diff;

    if (me->IsNonMeleeSpellCasted(false))
        return;

    // If have target
    if (victim)
    {
        // attack
        me->SetInFront(victim);                         // client change orientation by self
        me->CastSpell(victim, me->ToTotem()->GetSpell(), false);
    }
    else
        i_victimGuid = 0;
}

void TotemAI::AttackStart(Unit *)
{
    // Sentry totem sends ping on attack
    if (me->GetEntry() == SENTRY_TOTEM_ENTRY && me->GetOwner()->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(MSG_MINIMAP_PING, (8+4+4));
        data << me->GetGUID();
        data << me->GetPositionX();
        data << me->GetPositionY();
        ((Player*)me->GetOwner())->GetSession()->SendPacket(&data);
    }
}

bool TotemAI::CheckCurrentTarget(Unit* victim)
{
    return victim && victim->isTargetableForAttack() && me->IsWithinDistInMap(victim, m_maxRange)
        && !me->IsFriendlyTo(victim) && victim->isVisibleForOrDetect(me, false);
}

Unit* TotemAI::UpdateTarget()
{
    Unit* victim = NULL;

    if (me->GetEntry() == SEARING_TOTEM_ENTRY)
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(me->ToTotem()->GetSpell());
        std::list<Unit*> tmpTargetList;
        Position pos;
        me->GetPosition(&pos);

        Trinity::SpellNotifierCreatureAndPlayer notifier(me, tmpTargetList, m_maxRange, PUSH_SRC_CENTER, spellInfo, SPELL_TARGETS_ENEMY, &pos, 0, false);
        me->GetMap()->VisitAll(pos.m_positionX, pos.m_positionY, m_maxRange, notifier);

        float cachedRange = m_maxRange + 1.0f, tmpRange;
        bool foundPreferred = false, tmpPreferred;
        uint32 cachedAura = 0, tmpAura = 0;
        for (std::list<Unit*>::iterator itr = tmpTargetList.begin(); itr != tmpTargetList.end(); ++itr)
        {
            tmpRange = (*itr)->GetDistance(me);

            // Searing Totem prefers targets with owners Flame Shock or Stormstrike aura
            if ((*itr)->HasAura(MASTER_SPELL_FLAME_SHOCK, me->GetOwnerGUID()))
                tmpAura = MASTER_SPELL_FLAME_SHOCK;
            else if ((*itr)->HasAura(MASTER_SPELL_STORMSTRIKE, me->GetOwnerGUID()))
                tmpAura = MASTER_SPELL_STORMSTRIKE;
            else
                tmpAura = 0;

            // if temp aura == 0, then target is not capable by present aura
            tmpPreferred = (tmpAura != 0);
            if ((!foundPreferred && tmpPreferred) || (!foundPreferred && tmpRange < cachedRange) || (foundPreferred && tmpPreferred && tmpRange < cachedRange))
            {
                // if already found some target with Stormstrike, select nearest of Stormstriked targets
                // if not, select any other target
                if (cachedAura != MASTER_SPELL_STORMSTRIKE || tmpAura == MASTER_SPELL_STORMSTRIKE)
                {
                    if (CheckCurrentTarget(*itr))
                    {
                        victim = *itr;
                        cachedRange = tmpRange;
                        foundPreferred = tmpPreferred;
                        cachedAura = tmpAura;
                    }
                }
            }
        }

        i_targetSearchTimer = 2000;
    }
    else
    {
        Trinity::NearestAttackableUnitInObjectRangeCheck u_check(me, me, m_maxRange);
        Trinity::UnitLastSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> checker(me, victim, u_check);
        me->VisitNearbyObject(m_maxRange, checker);

        i_targetSearchTimer = DEFAULT_TOTEM_TARGET_SEARCH_TIMER;
    }

    if (victim)
        i_victimGuid = victim->GetGUID();

    return victim;
}
