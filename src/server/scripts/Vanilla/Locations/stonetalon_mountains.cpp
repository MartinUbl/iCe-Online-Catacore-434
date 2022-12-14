/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Stonetalon_Mountains
SD%Complete: 95
SDComment: Quest support: 6627, 6523
SDCategory: Stonetalon Mountains
EndScriptData */

/* ContentData
npc_braug_dimspirit
npc_kaya_flathoof
EndContentData */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"

/*######
## npc_braug_dimspirit
######*/

#define GOSSIP_HBD1 "Ysera"
#define GOSSIP_HBD2 "Neltharion"
#define GOSSIP_HBD3 "Nozdormu"
#define GOSSIP_HBD4 "Alexstrasza"
#define GOSSIP_HBD5 "Malygos"

class npc_braug_dimspirit : public CreatureScript
{
public:
    npc_braug_dimspirit() : CreatureScript("npc_braug_dimspirit") { }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            pPlayer->CLOSE_GOSSIP_MENU();
            pCreature->CastSpell(pPlayer,6766,false);

        }
        if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
        {
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->AreaExploredOrEventHappens(6627);
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->GetQuestStatus(6627) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HBD1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HBD2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HBD3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HBD4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HBD5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

            pPlayer->SEND_GOSSIP_MENU(5820, pCreature->GetGUID());
        }
        else
            pPlayer->SEND_GOSSIP_MENU(5819, pCreature->GetGUID());

        return true;
    }

};


/*######
## npc_kaya_flathoof
######*/

enum eKaya
{
    FACTION_ESCORTEE_H          = 775,

    NPC_GRIMTOTEM_RUFFIAN       = 11910,
    NPC_GRIMTOTEM_BRUTE         = 11912,
    NPC_GRIMTOTEM_SORCERER      = 11913,

    SAY_START                   = -1000357,
    SAY_AMBUSH                  = -1000358,
    SAY_END                     = -1000359,

    QUEST_PROTECT_KAYA          = 6523
};

class npc_kaya_flathoof : public CreatureScript
{
public:
    npc_kaya_flathoof() : CreatureScript("npc_kaya_flathoof") { }

    struct npc_kaya_flathoofAI : public npc_escortAI
    {
        npc_kaya_flathoofAI(Creature* c) : npc_escortAI(c) {}

        void WaypointReached(uint32 i)
        {
            Player* pPlayer = GetPlayerForEscort();

            if (!pPlayer)
                return;

            switch(i)
            {
            case 16:
                DoScriptText(SAY_AMBUSH, me);
                me->SummonCreature(NPC_GRIMTOTEM_BRUTE, -48.53f, -503.34f, -46.31f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                me->SummonCreature(NPC_GRIMTOTEM_RUFFIAN, -38.85f, -503.77f, -45.90f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                me->SummonCreature(NPC_GRIMTOTEM_SORCERER, -36.37f, -496.23f, -45.71f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                break;
            case 18: me->SetInFront(pPlayer);
                DoScriptText(SAY_END, me, pPlayer);
                if (pPlayer)
                    pPlayer->GroupEventHappens(QUEST_PROTECT_KAYA, me);
                break;
            }
        }

        void JustSummoned(Creature* summoned)
        {
            summoned->AI()->AttackStart(me);
        }

        void Reset(){}
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_PROTECT_KAYA)
        {
            if (npc_escortAI* pEscortAI = CAST_AI(npc_kaya_flathoof::npc_kaya_flathoofAI, pCreature->AI()))
                pEscortAI->Start(true, false, pPlayer->GetGUID());

            DoScriptText(SAY_START, pCreature);
            pCreature->setFaction(113);
            pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_kaya_flathoofAI(pCreature);
    }

};

/////////////////////////////////////////////////////////////////////////////////////
// Velkej Gregovo Stonetalon Mountains event                                       //
/////////////////////////////////////////////////////////////////////////////////////

class StonetalonBombMapScript: public WorldMapScript
{
    public:
        StonetalonBombMapScript(): WorldMapScript("stonetalon_bomb_map", 731)
        {
        }

        Player* pLeaders[2];
        std::vector<Player*> PlayerMap[2];

        void OnCreate(Map* pMap)
        {
            for (int i = 0; i < 1; i++)
            {
                pLeaders[i] = NULL;
                PlayerMap[i].clear();
            }
        }

        void MakeLeader(TeamId team, Player* pPlayer)
        {
            pLeaders[team] = pPlayer;
            pPlayer->CastSpell(pPlayer, 50771, true);
            pPlayer->CastSpell(pPlayer, 37964, true);
        }

        void MakeNewLeader(TeamId team)
        {
            if (PlayerMap[team].empty())
            {
                pLeaders[team] = NULL;
                return;
            }

            uint32 which = urand(0,PlayerMap[team].size()-1);
            if (PlayerMap[team][which])
                pLeaders[team] = PlayerMap[team][which];
            else
                pLeaders[team] = NULL;
        }

        void DeletePlayerFromList(Player* pPlayer)
        {
            if (!pPlayer)
                return;

            //std::list<Player*>::const_iterator itr = PlayerMap[pPlayer->GetTeamId()].find(pPlayer);
            //if (itr != PlayerMap[pPlayer->GetTeamId()].end())
            //    PlayerMap[pPlayer->GetTeamId()].erase(itr);
        }

        void OnPlayerEnter(Map* pMap, Player* pPlayer)
        {
            if (pPlayer)
            {
                if (pPlayer->HasAura(50771) && pPlayer != pLeaders[pPlayer->GetTeamId()])
                    pPlayer->RemoveAurasDueToSpell(50771);

                PlayerMap[pPlayer->GetTeamId()].push_back(pPlayer);

                if (pPlayer->GetTeamId() == TEAM_HORDE)
                {
                    // HORDE
                    if (!pLeaders[TEAM_HORDE])
                    {
                        MakeLeader(TEAM_HORDE, pPlayer);
                    }
                    pPlayer->NearTeleportTo(2047.04382f, 1172.4026f, 321.5933f, 4.32f);
                }
                else
                {
                    // ALLIANCE
                    if (!pLeaders[TEAM_ALLIANCE])
                    {
                        MakeLeader(TEAM_ALLIANCE, pPlayer);
                    }
                    pPlayer->NearTeleportTo(2074.0258f, 1601.9565f, 341.2546f, 2.39f);
                }
            }
        }

        void OnPlayerLeave(Map* pMap, Player* pPlayer)
        {
            if (pPlayer)
            {
                DeletePlayerFromList(pPlayer);

                if (pPlayer == pLeaders[pPlayer->GetTeamId()])
                    MakeNewLeader(pPlayer->GetTeamId());
            }
        }
};

/*######
## AddSC
######*/

void AddSC_stonetalon_mountains()
{
    new npc_braug_dimspirit();
    new npc_kaya_flathoof();
    new StonetalonBombMapScript();
}
