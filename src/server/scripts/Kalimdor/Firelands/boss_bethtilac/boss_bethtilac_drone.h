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
 * class declaration for Cinderweb Drone AI
 */


#ifndef BOSS_BETHTILAC_DRONE_H
#define BOSS_BETHTILAC_DRONE_H


#include "boss_bethtilac_spiderAI.h"


class mob_drone: public CreatureScript
{
public:
    mob_drone(): CreatureScript("mob_cinderweb_drone") {}
    CreatureAI *GetAI(Creature *creature) const;

private:
    class mob_droneAI: public SpiderAI
    {
    public:
        explicit mob_droneAI(Creature *creature);
        virtual ~mob_droneAI();

    private:
        // virtual method overrides
        void Reset();
        void EnterCombat(Unit *who);
        //bool UpdateVictim();
        //void DamageTaken(Unit *attacker, uint32 &damage);
        void EnterEvadeMode();
        //void KilledUnit(Unit *victim);
        //void JustDied(Unit *killer);
        void UpdateAI(const uint32 diff);
        void DoAction(const int32 event);
        void MovementInform(uint32 type, uint32 id);
        //void SummonedCreatureDespawn(Creature *creature);
        //void MoveInLineOfSight(Unit *who);
        void AttackStart(Unit *victim);
        //void JustRespawned();
        void IsSummonedBy(Unit *summoner);

        // attributes
        bool onGround;
        bool onTop;
    };
};


#endif // BOSS_BETHTILAC_DRONE_H
