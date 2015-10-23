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

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "TaskScheduler.h"

///////////////////////////
///    Morchok stuff    ///
///////////////////////////

///////////////////////////
///    Zonozz stuff     ///
///////////////////////////

///////////////////////////
///    Yorsahj stuff    ///
///////////////////////////

///////////////////////////
///    Hagara stuff     ///
///////////////////////////

///////////////////////////
///   Ultraxion stuff   ///
///////////////////////////

///////////////////////////
///   Blackhorn stuff   ///
///////////////////////////

///////////////////////////
///     Spine stuff     ///
///////////////////////////

///////////////////////////
///    Madness stuff    ///
///////////////////////////

////////////////////////////
/// Other instance stuff ///
////////////////////////////

static const Position telePos[6] =
{
    {  -1779.50f,  -2393.43f,  45.61f, 3.20f }, // Wyrmrest Temple/Base
    {  -1804.10f,  -2402.60f, 341.35f, 0.48f }, // Wyrmrest Summit
    {  13629.35f,  13612.09f, 123.49f, 3.14f }, // Hagara
    {  13444.90f, -12133.30f, 151.21f, 0.00f }, // Skifire (Ship)
    { -13852.50f, -13665.38f, 297.37f, 1.53f }, // Spine of Deathwing
    { -12081.39f,  12160.05f,  30.60f, 6.03f }, // Madness of Deathwing
};

class go_ds_instance_teleporter : public GameObjectScript
{
public:
    go_ds_instance_teleporter() : GameObjectScript("go_ds_instance_teleporter") { }

    bool OnGossipHello(Player* pPlayer, GameObject* pGameObject)
    {
        if (InstanceScript* pInstance = pGameObject->GetInstanceScript())
        {
            // Disable teleport if encounter in progress
            for (uint8 i = 0; i < MAX_ENCOUNTER; i++)
            {
                if (pInstance->GetData(i) == IN_PROGRESS)
                    return true;
            }

            switch (pGameObject->GetEntry())
            {
                // Base Teleporter near Entrance
            case GO_TRAVEL_TO_WYRMREST_TEMPLE:
                if (pInstance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) == DONE)
                    pPlayer->NearTeleportTo(telePos[5].GetPositionX(), telePos[5].GetPositionY(), telePos[5].GetPositionZ(), telePos[5].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_BLACKHORN) == DONE || pInstance->GetData(TYPE_BOSS_ULTRAXION) == DONE)
                    pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_HAGARA) == DONE
                    || (pInstance->GetData(TYPE_BOSS_ZONOZZ) == DONE && pInstance->GetData(TYPE_BOSS_YORSAHJ) == DONE))
                    pPlayer->NearTeleportTo(telePos[1].GetPositionX(), telePos[1].GetPositionY(), telePos[1].GetPositionZ(), telePos[1].GetOrientation());
                else if (pInstance->GetData(TYPE_BOSS_MORCHOK) == DONE)
                    pPlayer->NearTeleportTo(telePos[0].GetPositionX(), telePos[0].GetPositionY(), telePos[0].GetPositionZ(), telePos[0].GetOrientation());
                break;
                // Zonozz and Yorsahj teleport back to wyrmrest temple
            case GO_TRAVEL_TO_WYRMREST_BASE:
                pPlayer->NearTeleportTo(telePos[0].GetPositionX(), telePos[0].GetPositionY(), telePos[0].GetPositionZ(), telePos[0].GetOrientation());
                break;
                // Hagara back teleporter
            case GO_TRAVEL_TO_WYRMREST_SUMMIT:
                pPlayer->NearTeleportTo(telePos[1].GetPositionX(), telePos[1].GetPositionY(), telePos[1].GetPositionZ(), telePos[1].GetOrientation());
                break;
                // Hagara teleporter
            case GO_TRAVEL_TO_EYE_OF_ETERNITY:
                if (pInstance->GetData(TYPE_BOSS_ZONOZZ) == DONE)
                    pPlayer->NearTeleportTo(telePos[2].GetPositionX(), telePos[2].GetPositionY(), telePos[2].GetPositionZ(), telePos[2].GetOrientation());
                break;
                // Skifire teleporter
            case GO_TRAVEL_TO_DECK:
                if (pInstance->GetData(TYPE_BOSS_ULTRAXION) == DONE)
                    pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                break;
                // Maelstrom teleporter
            case GO_TRAVEL_TO_MAELSTROM:
                if (pInstance->GetData(TYPE_BOSS_SPINE_OF_DEATHWING) == DONE)
                    pPlayer->NearTeleportTo(telePos[6].GetPositionX(), telePos[6].GetPositionY(), telePos[6].GetPositionZ(), telePos[6].GetOrientation());
                break;
            default:
                break;
            }

        }
        return true;
    }
};

class npc_ds_instance_teleporter : public CreatureScript
{
public:
    npc_ds_instance_teleporter() : CreatureScript("npc_ds_instance_teleporter") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_instance_teleporterAI(pCreature);
    }

    struct npc_ds_instance_teleporterAI : public ScriptedAI
    {
        npc_ds_instance_teleporterAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            if (instance && !(instance->GetData(TYPE_BOSS_MORCHOK) == DONE))
                me->SetVisible(false);
            if (instance && (instance->GetData(TYPE_BOSS_HAGARA) == DONE) && (me->GetEntry() == NPC_TRAVEL_TO_EYE_OF_ETERNITY))
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript * instance;

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell) override
        {
            if (spell->Id == SPELL_OPEN_EYE_OF_ETERNITY_PORTAL)
            {
                me->SetVisible(true);
                me->SummonGameObject(GO_TRAVEL_TO_EYE_OF_ETERNITY, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 3, me->GetOrientation(), 0, 0, 0, 0, 604800);
            }
        }
    };
};

#define GOSSIP_GUNSHIP_READY                       "We are the Alliance. We are always ready."
#define GOSSIP_START_BLACKHORN_ENCOUNTER           "Bring us in closer!"
#define GOSSIP_SPINE_OF_DEATHWING                  "JUSTICE AND GLORY!"

enum SkyfireNpc
{
    NPC_BOSS_BLACKHORN                  = 56427,
};

enum gunshipActions
{
    ACTION_READY_TO_FOLLOW_DEATHWING    = 0,
    ACTION_START_BLACKHORN_ENCOUNTER    = 1,
    ACTION_SPINE_OF_DEATHWING           = 2,
};

class npc_ds_alliance_ship_crew : public CreatureScript
{
public:
    npc_ds_alliance_ship_crew() : CreatureScript("npc_ds_alliance_ship_crew") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pCreature->GetInstanceScript()->GetData(TYPE_BOSS_BLACKHORN) == DONE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SPINE_OF_DEATHWING, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        else if (Creature * pBlackhorn = pCreature->FindNearestCreature(NPC_BOSS_BLACKHORN, 200.0f, true))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_START_BLACKHORN_ENCOUNTER, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        }
        else if (pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 3)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_GUNSHIP_READY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            if (pCreature->GetInstanceScript()->GetData(DATA_ASPECTS_PREPARE_TO_CHANNEL) == 3)
                pCreature->AI()->DoAction(ACTION_READY_TO_FOLLOW_DEATHWING);
            else
                pPlayer->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
        }
        else if (uiAction == GOSSIP_ACTION_INFO_DEF + 2)
        {
            pCreature->AI()->DoAction(ACTION_START_BLACKHORN_ENCOUNTER);
        }
        else if (uiAction == GOSSIP_ACTION_AUCTION + 3)
        {
            pCreature->AI()->DoAction(ACTION_SPINE_OF_DEATHWING);
        }
        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_alliance_ship_crewAI(pCreature);
    }

    struct npc_ds_alliance_ship_crewAI : public ScriptedAI
    {
        npc_ds_alliance_ship_crewAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            if (instance && instance->GetData(TYPE_BOSS_ULTRAXION) != DONE)
                me->SetVisible(false);
        }

        InstanceScript * instance;
        TaskScheduler scheduler;

        void Reset() override
        {
            if (me->GetEntry() == NPC_SKY_CAPTAIN_SWAYZE)
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void TeleportAllPlayers(uint32 action, bool withParachute)
        {
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (!player->IsGameMaster())
                    {
                        if (action == ACTION_READY_TO_FOLLOW_DEATHWING)
                            player->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());
                        else if (action == ACTION_SPINE_OF_DEATHWING)
                            player->NearTeleportTo(telePos[3].GetPositionX(), telePos[3].GetPositionY(), telePos[3].GetPositionZ(), telePos[3].GetOrientation());

                        if (withParachute == true)
                            player->AddAura(SPELL_PARACHUTE, player);
                    }
                }
            }
        }

        void PlayMovieToPlayers(uint32 movieId)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();
            if (plList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * pPlayer = itr->getSource())
                    pPlayer->SendMovieStart(movieId);
            }
        }

        void DoAction(const int32 action) override
        {
            if (instance)
            {
                if (action == ACTION_READY_TO_FOLLOW_DEATHWING)
                {
                    TeleportAllPlayers(action, false);
                    instance->SetData(DATA_ASPECTS_PREPARE_TO_CHANNEL, 4);
                }
                else if (action == ACTION_START_BLACKHORN_ENCOUNTER)
                {
                    instance->SetData(DATA_START_BLACKHORN_ENCOUNTER, 0);
                }
                else if (action == ACTION_SPINE_OF_DEATHWING)
                {
                    //PlayMovieToPlayers(ID);
                    scheduler.Schedule(Seconds(11), [this](TaskContext /*Show Movie and teleport to Spine of Deathwing*/)
                    {
                        TeleportAllPlayers(ACTION_SPINE_OF_DEATHWING, true);
                    });
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);
        }
    };
};

void AddSC_dragon_soul_trash()
{
    new npc_ds_instance_teleporter();
    new go_ds_instance_teleporter();
    new npc_ds_alliance_ship_crew();
}