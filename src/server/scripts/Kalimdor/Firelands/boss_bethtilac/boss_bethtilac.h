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
 * class declaration for Beth'tilac AI
 */


#ifndef BOSS_BETHTILAC_H
#define BOSS_BETHTILAC_H


#include "boss_bethtilac_spiderAI.h"

enum BethtilacPhases { PHASE_IDLE, PHASE_TRANSFER_1, PHASE_1, PHASE_TRANSFER_2, PHASE_2 };


// definition of AI class used by Beth'tilac (Firelands)

class boss_bethtilac: public CreatureScript
{
public:
    boss_bethtilac() : CreatureScript("boss_bethtilac") {}
    CreatureAI *GetAI(Creature *creature) const { return new boss_bethtilacAI(creature); }

private:
    class boss_bethtilacAI: public SpiderAI
    {
    public:
        explicit boss_bethtilacAI(Creature *creature);
        virtual ~boss_bethtilacAI();

    private:
        // virtual method overrides
        void Reset();
        void EnterCombat(Unit *who);
        bool UpdateVictim();
        void DamageTaken(Unit *attacker, uint32 &damage);
        void EnterEvadeMode();
        void KilledUnit(Unit *victim);
        void JustDied(Unit *killer);
        void UpdateAI(const uint32 diff);
        void DoAction(const int32 event);
        void MovementInform(uint32 type, uint32 id);
        //void SummonedCreatureDespawn(Creature *creature);
        void MoveInLineOfSight(Unit *who);
        void AttackStart(Unit *victim);

        // attributes
        BethtilacPhases phase, oldPhase;
        bool devastationEnabled;    // Smoldering devastation is disabled for a while after cast to avoid duplicate casts
        bool combatCheckEnabled;
        int devastationCounter;
        int spinnerCounter;         // number of the vawe of spinners

        // methods
        void SetPhase(BethtilacPhases newPhase);
        void EnterPhase(BethtilacPhases newPhase);
        void ScheduleEventsForPhase(BethtilacPhases phase);

        bool IsInTransfer();
        void ResetPower();
        void DepletePower();

        void DoSmolderingDevastation();

        void ShowWarnText(const char *text);    // show warning text in the client screen (checked by videos)

        GameObject *FindDoor();
        void LockDoor();
        void UnlockDoor();

        // spawns
        void SummonDrone();
        void SummonSpinners(bool withWarn);
        void SummonSpiderlings();
    };
};


#endif // BOSS_BETHTILAC_H
