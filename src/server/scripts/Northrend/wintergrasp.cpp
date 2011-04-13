/* Copyright (C) 2008 - 2009 Trinity <http://www.trinitycore.org/>
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

#include "ScriptPCH.h"
#include "BattlefieldMgr.h"
#include "BattlefieldWG.h"
#include "Battlefield.h"
#include "ScriptSystem.h"
#include "WorldSession.h"
#include "ObjectMgr.h"

#define GOSSIP_HELLO_DEMO1  "Build catapult."
#define GOSSIP_HELLO_DEMO2  "Build demolisher."
#define GOSSIP_HELLO_DEMO3  "Build siege engine."
#define GOSSIP_HELLO_DEMO4  "I cannot build more!"

enum eWGqueuenpctext
{
    WG_NPCQUEUE_TEXT_H_NOWAR = 14775,
    WG_NPCQUEUE_TEXT_H_QUEUE = 14790,
    WG_NPCQUEUE_TEXT_H_WAR   = 14777,
    WG_NPCQUEUE_TEXT_A_NOWAR = 14782,
    WG_NPCQUEUE_TEXT_A_QUEUE = 14791,
    WG_NPCQUEUE_TEXT_A_WAR   = 14781,
    WG_NPCQUEUE_TEXTOPTION_JOIN = -1850507,
};

class npc_demolisher_engineerer : public CreatureScript
{
public:
    npc_demolisher_engineerer() : CreatureScript("npc_demolisher_engineerer") { }
    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(1);

        if (!BfWG)
            return true;

		uint32 MaxVeh = BfWG->GetData((pCreature->IsFriendlyTo(pPlayer) && pPlayer->GetTeamId() == TEAM_HORDE)?BATTLEFIELD_WG_DATA_MAX_VEHICLE_H:BATTLEFIELD_WG_DATA_MAX_VEHICLE_A);
		if(MaxVeh == -1)
			MaxVeh = 0;

		uint32 ActVeh = BfWG->GetData((pCreature->IsFriendlyTo(pPlayer) && pPlayer->GetTeamId() == TEAM_HORDE) ?BATTLEFIELD_WG_DATA_VEHICLE_H:BATTLEFIELD_WG_DATA_VEHICLE_A);
		if(ActVeh == -1 || ActVeh > 500)
			ActVeh = 0;

        if(MaxVeh > ActVeh)
		{
            if (pPlayer->HasAura(SPELL_CORPORAL))
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO1, GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF);
            else if (pPlayer->HasAura(SPELL_LIEUTENANT))
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO1, GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO2, GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF+1);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO3, GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF+2);
            }
        }
        else
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO4, GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF+9);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(1);

        if (!BfWG)
            return true;

		uint32 MaxVeh = BfWG->GetData((pCreature->IsFriendlyTo(pPlayer) && pPlayer->GetTeamId() == TEAM_HORDE)?BATTLEFIELD_WG_DATA_MAX_VEHICLE_H:BATTLEFIELD_WG_DATA_MAX_VEHICLE_A);
		if(MaxVeh == -1)
			MaxVeh = 0;

		uint32 ActVeh = BfWG->GetData((pCreature->IsFriendlyTo(pPlayer) && pPlayer->GetTeamId() == TEAM_HORDE)?BATTLEFIELD_WG_DATA_VEHICLE_H:BATTLEFIELD_WG_DATA_VEHICLE_A);
		if(ActVeh == -1 || ActVeh > 500)
			ActVeh = 0;

        if(MaxVeh > ActVeh)
		{
			switch(uiAction - GOSSIP_ACTION_INFO_DEF)
            {
                case 0: pPlayer->CastSpell(pPlayer, 56663, false, NULL, NULL, pCreature->GetGUID()); break;
                case 1: pPlayer->CastSpell(pPlayer, 56575, false, NULL, NULL, pCreature->GetGUID()); break;
                case 2: pPlayer->CastSpell(pPlayer, pPlayer->GetTeamId() ? 61408 : 56661, false, NULL, NULL, pCreature->GetGUID()); break;
            }
            //spell 49899 Emote : 406 from sniff
            //INSERT INTO `spell_scripts` (`id`, `delay`, `command`, `datalong`, `datalong2`, `dataint`, `x`, `y`, `z`, `o`) VALUES ('49899', '0', '1', '406', '0', '0', '0', '0', '0', '0');
            if(Creature *creature = pCreature->FindNearestCreature(27852,30.0f,true))
                creature->CastSpell(creature,49899,true);
        }
        return true;
    }
};

class npc_wg_spiritguide : public CreatureScript
{
public:
    npc_wg_spiritguide() : CreatureScript("npc_wg_spiritguide") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
        if (BfWG)
        {
            GraveYardVect gy=BfWG->GetGraveYardVect();
            for(uint8 i=0;i<gy.size();i++)
            {
                if(gy[i]->GetControlTeamId()==pPlayer->GetTeamId())
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT,sObjectMgr->GetTrinityStringForDBCLocale(((BfGraveYardWG*)gy[i])->GetTextId()), GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF+i);
                }
            }
        }

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
        if (BfWG)
        {
            GraveYardVect gy=BfWG->GetGraveYardVect();
            for(uint8 i=0;i<gy.size();i++)
            {
                if(uiAction-GOSSIP_ACTION_INFO_DEF==i && gy[i]->GetControlTeamId()==pPlayer->GetTeamId())
                {
                    const WorldSafeLocsEntry* ws=sWorldSafeLocsStore.LookupEntry(gy[i]->GetGraveYardId());
                    pPlayer->TeleportTo(ws->map_id,ws->x,ws->y,ws->z,0);
                }
            }
        }
        return true;
    }
};

class npc_wg_dalaran_queue : public CreatureScript
{
public:
    npc_wg_dalaran_queue() : CreatureScript("npc_wg_dalaran_queue") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
        if (BfWG)
        {

            if(BfWG->IsWarTime())
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT,sObjectMgr->GetTrinityStringForDBCLocale(WG_NPCQUEUE_TEXTOPTION_JOIN), GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF);
                pPlayer->SEND_GOSSIP_MENU(BfWG->GetDefenderTeam()?WG_NPCQUEUE_TEXT_H_WAR:WG_NPCQUEUE_TEXT_A_WAR, pCreature->GetGUID());
            }
            else
            {
                uint32 uiTime=(uint32)BfWG->GetTimer()/1000;
                pPlayer->SendUpdateWorldState(4354,(uint32)time(NULL)+uiTime);
                if(uiTime<15*MINUTE)
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT,sObjectMgr->GetTrinityStringForDBCLocale(WG_NPCQUEUE_TEXTOPTION_JOIN), GOSSIP_SENDER_MAIN,   GOSSIP_ACTION_INFO_DEF);
                    pPlayer->SEND_GOSSIP_MENU(BfWG->GetDefenderTeam()?WG_NPCQUEUE_TEXT_H_QUEUE:WG_NPCQUEUE_TEXT_A_QUEUE, pCreature->GetGUID());
                }
                else
                {
                    pPlayer->SEND_GOSSIP_MENU(BfWG->GetDefenderTeam()?WG_NPCQUEUE_TEXT_H_NOWAR:WG_NPCQUEUE_TEXT_A_NOWAR, pCreature->GetGUID());
                }
            }
        }
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 /*uiAction*/)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        BattlefieldWG *BfWG = (BattlefieldWG*)sBattlefieldMgr.GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
        if (BfWG)
        {
            if(BfWG->IsWarTime()){
                pPlayer->GetSession()->SendBfInvitePlayerToWar(BATTLEFIELD_BATTLEID_WG,4197,20);
            }
            else
            {
                uint32 uiTime=(uint32)BfWG->GetTimer()/1000;
                if(uiTime<15*MINUTE)
                    pPlayer->GetSession()->SendBfInvitePlayerToQueue(BATTLEFIELD_BATTLEID_WG);
            }
        }
        return true;
    }
};

struct BfWGCoordGOTele
{
	float x;
	float y;
	float z;
	float o;
};
#define BATTLEFIELD_WG_GO_MAX 2
const BfWGCoordGOTele WGGOTele[BATTLEFIELD_WG_GO_MAX]=
{
	{5249.890137f,2703.110107f,409.2749f,2.37f},
	{5246.92f,2978.32f,409.274f,3.86f},
};
#define BATTLEFIELD_WG_VE_MAX 4
const uint32 entry_vehicle[BATTLEFIELD_WG_VE_MAX]=
{
	28094,
	27881,
	32627,
	28312,
};

class npc_wintergrasp_teleporter : public CreatureScript
{
public:
    npc_wintergrasp_teleporter() : CreatureScript("npc_wintergrasp_teleporter") { }

	struct npc_wintergrasp_teleporterAI : public ScriptedAI
    {
		npc_wintergrasp_teleporterAI(Creature *c) : ScriptedAI(c) {}

		void UpdateAI(const uint32 /*diff*/)
		{
			for (int8 i = 0;i<BATTLEFIELD_WG_VE_MAX;i++)
			{
				Creature *ve = me->FindNearestCreature(entry_vehicle[i], 5);
				if (ve && ve->IsVehicle() && ve->GetVehicleKit()->IsVehicleInUse())
				{
					Battlefield *BfWG = sBattlefieldMgr.GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
					Unit *player = ve->GetVehicleKit()->GetPassenger(0);
					if (player && BfWG->GetDefenderTeam() == player->ToPlayer()->GetTeamId() && me->GetPositionY() > 2800)
					{
						ve->GetVehicleKit()->TeleportVehicle(WGGOTele[0].x, WGGOTele[0].y, WGGOTele[0].z, WGGOTele[0].o);
					}
					else if (player && BfWG->GetDefenderTeam() == player->ToPlayer()->GetTeamId() && me->GetPositionY() < 2800)
					{
						ve->GetVehicleKit()->TeleportVehicle(WGGOTele[1].x, WGGOTele[1].y, WGGOTele[1].z, WGGOTele[1].o);
					}
				}
			}
		}
	};

	CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wintergrasp_teleporterAI(creature);
    }
};

void AddSC_wintergrasp()
{
   new npc_wg_dalaran_queue();
   new npc_wg_spiritguide();
   new npc_demolisher_engineerer();
   new npc_wintergrasp_teleporter();
}
