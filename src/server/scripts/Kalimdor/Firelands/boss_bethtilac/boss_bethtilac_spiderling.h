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
 * class declaration for Cinderweb Spiderling AI
 */

#ifndef BOSS_BETHTILAC_SPIDERLING_H
#define BOSS_BETHTILAC_SPIDERLING_H


#include "boss_bethtilac_spiderAI.h"


class mob_spiderling: public CreatureScript
{
public:
    mob_spiderling(): CreatureScript("mob_cinderweb_spiderling") {}
    CreatureAI *GetAI(Creature *creature) const;

private:
    class mob_spiderlingAI: public SpiderAI
    {
    public:
        explicit mob_spiderlingAI(Creature *creature);
        virtual ~mob_spiderlingAI();

    private:
        // virtual method overrides

        //void Reset();
        //void EnterCombat(Unit *who);
        //bool UpdateVictim();
        //void DamageTaken(Unit *attacker, uint32 &damage);
        void EnterEvadeMode();
        //void KilledUnit(Unit *victim);
        //void JustDied(Unit *killer);
        void UpdateAI(const uint32 diff);
        void DoAction(const int32 event);
        void MovementInform(uint32 type, uint32 id);
        //void SummonedCreatureDespawn(Creature *creature);
        void MoveInLineOfSight(Unit *who);
        //void AttackStart(Unit *victim);
        //void JustRespawned();
        void IsSummonedBy(Unit *summoner);

        bool CanFollowTarget(Unit *target) const;

        // attributes
        uint64 followedGuid;
        bool following;

        // methods
        Unit *ChooseTarget();
        bool FollowTarget();
    };
};


#endif // BOSS_BETHTILAC_SPIDERLING_H
