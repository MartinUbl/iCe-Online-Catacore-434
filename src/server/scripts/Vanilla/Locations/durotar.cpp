/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptPCH.h"

/*######
##Quest 25134: Lazy Peons
##npc_lazy_peon
######*/

enum LazyPeonYells
{
    SAY_SPELL_HIT                                  = -1000600,
    SAY_SPELL_HIT2                                 = -1000601,
    SAY_SPELL_HIT3                                 = -1000602,
    SAY_SPELL_HIT4                                 = -1000603
};

enum LazyPeon
{
    QUEST_LAZY_PEONS                              = 25134,
    GO_LUMBERPILE                                 = 175784,
    SPELL_BUFF_SLEEP                              = 17743,
    SPELL_AWAKEN_PEON                             = 19938
};

class npc_lazy_peon : public CreatureScript
{
public:
    npc_lazy_peon() : CreatureScript("npc_lazy_peon") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_lazy_peonAI(pCreature);
    }

    struct npc_lazy_peonAI : public ScriptedAI
    {
        npc_lazy_peonAI(Creature *c) : ScriptedAI(c) {}

        uint64 uiPlayerGUID;

        uint32 m_uiRebuffTimer;
        bool work;

        void Reset ()
        {
            uiPlayerGUID = 0;
            m_uiRebuffTimer = 0;
            work = false;
        }

        void MovementInform(uint32 /*type*/, uint32 id)
        {
            if (id == 1)
                work = true;
        }

        void SpellHit(Unit *caster, const SpellEntry *spell)
        {
            if (spell->Id == SPELL_AWAKEN_PEON && caster->GetTypeId() == TYPEID_PLAYER
                && CAST_PLR(caster)->GetQuestStatus(QUEST_LAZY_PEONS) == QUEST_STATUS_INCOMPLETE)
            {
                caster->ToPlayer()->KilledMonsterCredit(me->GetEntry(),me->GetGUID());
                DoScriptText(RAND(SAY_SPELL_HIT,SAY_SPELL_HIT2,SAY_SPELL_HIT3,SAY_SPELL_HIT4), me, caster);
                me->RemoveAllAuras();
                if (GameObject* Lumberpile = me->FindNearestGameObject(GO_LUMBERPILE, 20))
                    me->GetMotionMaster()->MovePoint(1,Lumberpile->GetPositionX()-1,Lumberpile->GetPositionY(),Lumberpile->GetPositionZ());
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (work == true)
                me->HandleEmoteCommand(466);
            if (m_uiRebuffTimer <= uiDiff)
            {
                DoCast(me, SPELL_BUFF_SLEEP);
                m_uiRebuffTimer = 600000;                 //Rebuff agian in 1 minutes
            }
            else
                m_uiRebuffTimer -= uiDiff;
            if (!UpdateVictim())
                return;
            DoMeleeAttackIfReady();
        }
    };

};

/*
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (10556, -1000600, 'Ow! OK, I\'\'ll get back to work, $N!\'', '', '', '', '', '', '', '', '', 0, 0, 0, 0, '');
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (10556, -1000601, 'OK boss, I get back to tree-hitting.', '', '', '', '', '', '', '', '', 0, 0, 0, 0, '');
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (10556, -1000602, 'Just was resting eyes! Back to work now!', '', '', '', '', '', '', '', '', 0, 0, 0, 0, '');
INSERT INTO `script_texts` (`npc_entry`, `entry`, `content_default`, `content_loc1`, `content_loc2`, `content_loc3`, `content_loc4`, `content_loc5`, `content_loc6`, `content_loc7`, `content_loc8`, `sound`, `type`, `language`, `emote`, `comment`) VALUES (10556, -1000603, 'Sleepy... so sleepy...', '', '', '', '', '', '', '', '', 0, 0, 0, 0, '');

*/


void AddSC_durotar()
{
    new npc_lazy_peon();
}
