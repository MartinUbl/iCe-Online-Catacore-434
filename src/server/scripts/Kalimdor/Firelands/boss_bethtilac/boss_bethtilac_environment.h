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
 * declarations for environment scripts of Beth'tilac encounter
 */

#ifndef BOSS_BETHTILAC_EVNIRONMENT_H
#define BOSS_BETHTILAC_EVNIRONMENT_H


#include "boss_bethtilac_spiderAI.h"


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


class npc_web_rip: public CreatureScript
{
public:
    npc_web_rip(): CreatureScript("npc_web_rip") {}
    CreatureAI *GetAI(Creature *creature) const;

private:
    class AI: public ScriptedAI
    {
    public:
        explicit AI(Creature *creature);

    private:
        virtual void IsSummonedBy(Unit *summoner);
    };
};


// spell Meteor Burn - need to summon temporary NPC above the target, which casts the meteor
class spell_meteor_burn: public SpellScriptLoader
{
public:
    spell_meteor_burn(): SpellScriptLoader("spell_meteor_burn") {}
    SpellScript *GetSpellScript() const;

private:
    class Script: public SpellScript
    {
    private:
        PrepareSpellScript(Script)
        bool Validate(SpellEntry const *spell);
        void Register();

        void HandleHit(SpellEffIndex effIndex);
    };
};


#endif // BOSS_BETHTILAC_EVNIRONMENT_H
