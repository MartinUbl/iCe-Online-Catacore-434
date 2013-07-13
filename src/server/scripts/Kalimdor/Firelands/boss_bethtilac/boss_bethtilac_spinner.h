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
 * class declaration for Cinderweb Spinner AI
 */


#ifndef BOSS_BETHTILAC_SPINNER_H
#define BOSS_BETHTILAC_SPINNER_H


#include "boss_bethtilac_spiderAI.h"


class mob_spinner: public CreatureScript
{
public:
    mob_spinner(): CreatureScript("mob_cinderweb_spinner") {}
    CreatureAI *GetAI(Creature *creature) const;

private:
    class mob_spinnerAI: public SpiderAI
    {
    public:
        explicit mob_spinnerAI(Creature *creature);
        virtual ~mob_spinnerAI();

    private:
        // virtual method overrides
        //void Reset();
        //void EnterCombat(Unit *who);
        //bool UpdateVictim();
        //void DamageTaken(Unit *attacker, uint32 &damage);
        void EnterEvadeMode();
        //void KilledUnit(Unit *victim);
        void JustDied(Unit *killer);
        void UpdateAI(const uint32 diff);
        void DoAction(const int32 event);
        void MovementInform(uint32 type, uint32 id);
        //void SummonedCreatureDespawn(Creature *creature);
        //void MoveInLineOfSight(Unit *who);
        //void AttackStart(Unit *victim);
        //void JustRespawned();
        void IsSummonedBy(Unit * summoner);
        void SpellHit(Unit *caster, const SpellEntry *spell);

        bool hanging;       // spider is hanging on one place on the filament
        bool onGround;      // spider is moving on the ground (not hanging)
        bool summoned;  // summoned a filament for the players (either after taunted or killed)
    };
};


class npc_filament: public CreatureScript
{
public:
    npc_filament(): CreatureScript("npc_spiderweb_filament") {}
    CreatureAI *GetAI(Creature *creature) const;

private:
    // not quite a spider, but has same methods...
    class filamentAI: public SpiderAI
    {
    public:
        explicit filamentAI(Creature *creature);
        virtual ~filamentAI();

        void Reset();
        void UpdateAI(const uint32 diff);
        void MoveInLineOfSight(Unit *who);
        void AttackStart(Unit *victim);
        void EnterEvadeMode();
        void MovementInform(uint32 type, uint32 id);
        void PassengerBoarded(Unit *unit, int8 seat, bool apply);
        void DoAction(const int32 event);
    };
};


#endif // BOSS_BETHTILAC_SPINNER_H
