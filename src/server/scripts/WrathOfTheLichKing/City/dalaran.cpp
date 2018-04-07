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

/* Script Data Start
SDName: Dalaran
SDAuthor: WarHead, MaXiMiUS
SD%Complete: 99%
SDComment: For what is 63990+63991? Same function but don't work correct...
SDCategory: Dalaran
Script Data End */

#include "ScriptPCH.h"

/*******************************************************
 * npc_mageguard_dalaran
 *******************************************************/

enum Spells
{
    SPELL_TRESPASSER_A = 54028,
    SPELL_TRESPASSER_H = 54029
};

enum NPCs // All outdoor guards are within 35.0f of these NPCs
{
    NPC_APPLEBOUGH_A = 29547,
    NPC_SWEETBERRY_H = 29715,
};

class npc_mageguard_dalaran : public CreatureScript
{
public:
    npc_mageguard_dalaran() : CreatureScript("npc_mageguard_dalaran") { }

    struct npc_mageguard_dalaranAI : public Scripted_NoMovementAI
    {
        npc_mageguard_dalaranAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            pCreature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_NORMAL, true);
            pCreature->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, true);
        }

        void Reset(){}

        void EnterCombat(Unit* /*pWho*/){}

        void AttackStart(Unit* /*pWho*/){}

        void MoveInLineOfSight(Unit *pWho)
        {
            if (!pWho || !pWho->IsInWorld() || pWho->GetZoneId() != 4395)
                return;

            if (!me->IsWithinDist(pWho, 65.0f, false))
                return;

            Player *pPlayer = pWho->GetCharmerOrOwnerPlayerOrPlayerItself();

            if (!pPlayer || pPlayer->IsGameMaster() || pPlayer->IsBeingTeleported())
                return;

            switch (me->GetEntry())
            {
                case 29254:
                    if (pPlayer->GetTeam() == HORDE)              // Horde unit found in Alliance area
                    {
                        if (GetClosestCreatureWithEntry(me, NPC_APPLEBOUGH_A, 32.0f))
                        {
                            if (me->isInBackInMap(pWho, 12.0f))   // In my line of sight, "outdoors", and behind me
                                DoCast(pWho, SPELL_TRESPASSER_A); // Teleport the Horde unit out
                        }
                        else                                      // In my line of sight, and "indoors"
                            DoCast(pWho, SPELL_TRESPASSER_A);     // Teleport the Horde unit out
                    }
                    break;
                case 29255:
                    if (pPlayer->GetTeam() == ALLIANCE)           // Alliance unit found in Horde area
                    {
                        if (GetClosestCreatureWithEntry(me, NPC_SWEETBERRY_H, 32.0f))
                        {
                            if (me->isInBackInMap(pWho, 12.0f))   // In my line of sight, "outdoors", and behind me
                                DoCast(pWho, SPELL_TRESPASSER_H); // Teleport the Alliance unit out
                        }
                        else                                      // In my line of sight, and "indoors"
                            DoCast(pWho, SPELL_TRESPASSER_H);     // Teleport the Alliance unit out
                    }
                    break;
            }
            me->SetOrientation(me->GetHomePosition().GetOrientation());
            return;
        }

        void UpdateAI(const uint32 /*diff*/){}
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_mageguard_dalaranAI(creature);
    }
};

/*######
## npc_hira_snowdawn
######*/

enum eHiraSnowdawn
{
    SPELL_COLD_WEATHER_FLYING                   = 54197
};

#define GOSSIP_TEXT_TRAIN_HIRA "I seek training to ride a steed."

class npc_hira_snowdawn : public CreatureScript
{
public:
    npc_hira_snowdawn() : CreatureScript("npc_hira_snowdawn") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (!pCreature->IsVendor() || !pCreature->IsTrainer())
            return false;

        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_TRAIN_HIRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRAIN);

        if (pPlayer->getLevel() >= 80 && pPlayer->HasSpell(SPELL_COLD_WEATHER_FLYING))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_TRAIN)
            pPlayer->SEND_TRAINERLIST(pCreature->GetGUID());

        if (uiAction == GOSSIP_ACTION_TRADE)
            pPlayer->SEND_VENDORLIST(pCreature->GetGUID());

        return true;
    }
};

#define GOSSIP_TEXT_TRAIN_CHARLES "Train me."
#define GOSSIP_TEXT_TAILOR_DEATHCHILL_CLOAK_CHARLES "Tell me about the Deathchill Cloak Recipe."
#define GOSSIP_TEXT_TAILOR_WISPCLOAK_CHARLES "Tell me about the Wispcloak Recipe."
#define SKILL_ID_TAILORING 197
#define GOSSIP_MENU_TEXT_ID_CHARLES_W_TAILOR 14076
#define GOSSIP_MENU_TEXT_ID_CHARLES_DEATHCHILL_CLOAK 14074
#define GOSSIP_MENU_TEXT_ID_CHARLES_WISPCLOAK 14072
#define ACHIEV_ID_LOREMASTER_OF_NORTHREND 41 //Deathchill cloak
#define ACHIEV_ID_NORTHREND_DUNGEON_MASTER 1288 //Wispcloak
#define SPELL_ID_WISPCLOAK 56016
#define SPELL_ID_DEATHCHILL_CLOAK 56017

class npc_charles_worth : public CreatureScript
{
public:
    npc_charles_worth() : CreatureScript("npc_charles_worth") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (!pCreature->IsTrainer())
            return false;

        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        uint32 gossipMenuTextId = pPlayer->GetGossipTextId(pCreature);

        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_TEXT_TRAIN_CHARLES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRAIN);

        if (pPlayer->HasSkill(SKILL_ID_TAILORING))
        {
            gossipMenuTextId = GOSSIP_MENU_TEXT_ID_CHARLES_W_TAILOR;

            if (pPlayer->GetSkillValue(SKILL_ID_TAILORING) >= 420)
            {
                if (!pPlayer->HasSpell(SPELL_ID_DEATHCHILL_CLOAK))
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TEXT_TAILOR_DEATHCHILL_CLOAK_CHARLES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 0);

                if (!pPlayer->HasSpell(SPELL_ID_WISPCLOAK))
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_TEXT_TAILOR_WISPCLOAK_CHARLES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
        }

        pPlayer->SEND_GOSSIP_MENU(gossipMenuTextId, pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();

        const AchievementEntry* achiev = NULL;

        switch (uiAction)
        {
            case GOSSIP_ACTION_TRAIN:
            {
                pPlayer->SEND_TRAINERLIST(pCreature->GetGUID());
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 0: // Deathchill cloak
            {
                if ((achiev = sAchievementStore.LookupEntry(ACHIEV_ID_LOREMASTER_OF_NORTHREND)))
                {
                    if (pPlayer->GetAchievementMgr().HasAchieved(achiev))
                    {
                        pPlayer->LearnSpell(SPELL_ID_DEATHCHILL_CLOAK, false);
                        pPlayer->CLOSE_GOSSIP_MENU();
                    }
                    else
                        pPlayer->SEND_GOSSIP_MENU(GOSSIP_MENU_TEXT_ID_CHARLES_DEATHCHILL_CLOAK, pCreature->GetGUID());
                }
                break;
            }
            case GOSSIP_ACTION_INFO_DEF + 1: // Wispcloak
            {
                if ((achiev = sAchievementStore.LookupEntry(ACHIEV_ID_NORTHREND_DUNGEON_MASTER)))
                {
                    if (pPlayer->GetAchievementMgr().HasAchieved(achiev))
                    {
                        pPlayer->LearnSpell(SPELL_ID_WISPCLOAK, false);
                        pPlayer->CLOSE_GOSSIP_MENU();
                    }
                    else
                        pPlayer->SEND_GOSSIP_MENU(GOSSIP_MENU_TEXT_ID_CHARLES_WISPCLOAK, pCreature->GetGUID());
                }
                break;
            }
        }

        return true;
    }
};

void AddSC_dalaran()
{
    new npc_mageguard_dalaran;
    new npc_hira_snowdawn;
    new npc_charles_worth;
}
