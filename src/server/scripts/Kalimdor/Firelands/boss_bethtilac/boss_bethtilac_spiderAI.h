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
 * declaration of common ancestor class for creatures in Beth'tilac encounter
 */


#ifndef BOSS_BETHTILAC_SPIDERAI_H
#define BOSS_BETHTILAC_SPIDERAI_H


class SpiderAI: public ScriptedAI
{
private:
    struct Timer
    {
        int32 timer;
        int32 totalTime;
        int32 action;
        bool repeat;
    };

    typedef std::list<Timer> TimerList;
    TimerList timers;

public:
    SpiderAI(Creature *creature);
    virtual ~SpiderAI();

protected:
    // timers
    void AddTimer(int32 action, uint32 delay, bool repeat);
    //void RemoveTimer(int32 timerId);
    void ClearTimers();

    void UpdateTimers(const uint32 diff);

    // common functions
    void DebugOutput(const char *str) const;

    // manipulating Fire energy (for NPCs who do have it)
    uint32 GetPower() const;
    void SetMaxPower(uint32 newVal);
    void SetPower(uint32 newVal);
    void ModifyPower(int32 diff);

    // web filament (string)
    void SummonFilament();
    void UnSummonFilament();

    // movement
    void MoveToGround(uint32 movementId);
    void MoveToFilament(uint32 movementId);

    // virtual method overrides
    void SummonedCreatureDespawn(Creature *creature);
    void JustDied(Unit *killer);
    void MoveInLineOfSight(Unit *who);
    void SpellHit(Unit *victim, SpellEntry const *spell);
    void DoAction(const int32 event);

    // attributes
private:
    Creature *summonFilament;
protected:
    InstanceScript *instance;
};


#endif // BOSS_BETHTILAC_SPIDERAI_H
