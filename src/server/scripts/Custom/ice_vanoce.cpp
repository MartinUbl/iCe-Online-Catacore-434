/*
 * Copyright (C) 2006-2012 iCe Online <http://www.ice-wow.eu/>
 *
 * This program is not free software. iCe GM Team owns all of its content
 * Who won't obey that rule, i will kick his balls and twist his nipples.
 *
 */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"

/*######
## npc_duch minulosti
######*/

enum eEnums
{
    NPC_DUCH_MINULOSTI          = 500013, //KILL CREDIT
    NPC_DUCH_BUDOUCNOSTI        = 500014, //kill credit
    NPC_DUCH_PRITOMNOSTI        = 500015, // Kill Credit
    NPC_SOB                     = 500008,
    QUEST_ZACHRAN_DUCHY         = 500005,
};
#define GOSSIP_ITEM  "We can go"
class npc_duch_minulosti : public CreatureScript
{
public:
    npc_duch_minulosti() : CreatureScript("npc_duch_minulosti_escort") { }

    struct npc_duch_minulostiAI : public npc_escortAI
    {
        npc_duch_minulostiAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        void WaypointReached(uint32 i)
        {
            Player* pPlayer = GetPlayerForEscort();
            switch (i)
            {
                
                case 3:
                    me->SummonCreature(NPC_SOB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20000);
                    break;
                case 7:
                    for (uint32 i = 0; i < 2; i++)
                    me->SummonCreature(NPC_SOB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20000);
                    break;
                case 21:
                    for (uint32 i = 0; i < 15; i++)
                    me->SummonCreature(NPC_SOB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20000);
                break;
                case 13:
                    for (uint32 i = 0; i < 11; i++)
                    me->SummonCreature(NPC_SOB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 20000);
                    break;
                case 24:
                    me->MonsterSay("Dekuji ti tady už to zvladnu sam",0,0);
                    if (pPlayer)
                    {
                        pPlayer->KilledMonsterCredit(500013,0);
                    }
                    break;
            }
        }

        void Reset()  { }
        void JustDied(Unit* /*pKiller*/)
        {
            Player* pPlayer = GetPlayerForEscort();
            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
                if (pPlayer)
                    pPlayer->FailQuest(QUEST_ZACHRAN_DUCHY );
            }
        }

        void JustSummoned(Creature* summon)
            {
            Player* pPlayer = GetPlayerForEscort();
               switch(summon->GetEntry())
                {
                    case NPC_SOB:
                        if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 100.0f, true))
                        {
                            summon->SetLevel(pPlayer-> GetSession()->GetPlayer()->getLevel());
                           
                            summon->AI()->AttackStart(target);
                            summon->GetMotionMaster()->MoveChase(target);
                        }
                        break;
               }
                              
            }

        void UpdateAI(const uint32 uiDiff)
        {
            npc_escortAI::UpdateAI(uiDiff);

            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
              
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_duch_minulostiAI(creature);
    }

    bool OnGossipHello(Player *pPlayer, Creature *pCreature)
    {
        if (pPlayer->GetQuestStatus(QUEST_ZACHRAN_DUCHY ) == QUEST_STATUS_INCOMPLETE)
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        pPlayer->SEND_GOSSIP_MENU(1, pCreature->GetGUID());
        return true;

    }

 bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
        {
            player->PlayerTalkClass->ClearMenus();
            npc_duch_minulostiAI* pEscortAI = CAST_AI(npc_duch_minulosti::npc_duch_minulostiAI, creature->AI());

            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    player->CLOSE_GOSSIP_MENU();

                    if (pEscortAI)
                        pEscortAI->Start(false, false, player->GetGUID());
                    break;

                default:
                    return false;
            }

            return true;
        }
};



#define QUEST_JEDNOROZEC    500003
class npc_duch_minulosti_questgiver : public CreatureScript
{
public:
    npc_duch_minulosti_questgiver() : CreatureScript ("npc_duch_minulosti") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_JEDNOROZEC)
         {
           creature->MonsterWhisper("neboj se poslu te za jednorozcem",player->GetGUID());
           creature->AddAura(52275,player);
           player->SetPhaseMask(256,true);
           player->TeleportTo(0,-5432.0361f,1180.1640f,409.8187f,0.0f);
           player->SummonCreature(500003,-5432.0361f,1180.1640f,409.8187f,0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,4000);
           if (player)
           {
               player->GetSession()->GetPlayer()->MonsterSay("Ksakra kam mì to poslal",LANG_UNIVERSAL,0 );
           }
           return true;
        }
        return false;
   }

};

class npc_otrok: public CreatureScript
{
   public:
       npc_otrok(): CreatureScript("npc_otrok") {};
 
       struct npc_otrokAI: public ScriptedAI
       {
           npc_otrokAI(Creature* c): ScriptedAI(c)
           {
               Reset();
           }
 
           uint32 timer;
 
           void Reset()
           {
               timer = 15000;
           }
 
           void UpdateAI(const uint32 diff)
           {
               if (timer <= diff)
               {
                   switch(urand(0,3))
                   {
                       case 0:
                           me->MonsterYell("Prosimm uz dosssst",LANG_UNIVERSAL,0);
                           me->HandleEmoteCommand (EMOTE_ONESHOT_CRY);
                           break;
                       case 1:
                           me->MonsterSay("nee ja uz nemuzu",LANG_UNIVERSAL,0);
                           me->HandleEmoteCommand (EMOTE_ONESHOT_CRY);
                           break;
                       case 2:
                           me->MonsterYell("Nenavidim Vanoce!",LANG_UNIVERSAL,0);
                           me->HandleEmoteCommand (EMOTE_ONESHOT_LAUGH);
                           break;
                       case 3:
                           me->MonsterSay("Preji si aby jsi trpel bolesti Otrokari!",LANG_UNIVERSAL,0);
                           me->HandleEmoteCommand (EMOTE_ONESHOT_ROAR);
                           break;
                   }
                   timer = 40000;
               } else timer -= diff;
           }
       };
 
       CreatureAI* GetAI(Creature* c) const
       {
           return new npc_otrokAI(c);
       }
};


void AddSC_ice_vanoce()
{
    new npc_otrok();
    new npc_duch_minulosti_questgiver();
    new npc_duch_minulosti();
}

