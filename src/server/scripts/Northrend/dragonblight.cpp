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
SDName: Dragonblight
SD%Complete: 100
SDComment:
SDCategory: Dragonblight
EndScriptData */

/* ContentData
npc_alexstrasza_wr_gate
EndContentData */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"

enum eEnums
{
    QUEST_RETURN_TO_AG_A    = 12499,
    QUEST_RETURN_TO_AG_H    = 12500,
    MOVIE_ID_GATES          = 14
};

#define GOSSIP_ITEM_WHAT_HAPPENED   "Alexstrasza, can you show me what happened here?"

class npc_alexstrasza_wr_gate : public CreatureScript
{
public:
    npc_alexstrasza_wr_gate() : CreatureScript("npc_alexstrasza_wr_gate") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->IsQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->GetQuestRewardStatus(QUEST_RETURN_TO_AG_A) || pPlayer->GetQuestRewardStatus(QUEST_RETURN_TO_AG_H))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_WHAT_HAPPENED, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->SendMovieStart(MOVIE_ID_GATES);
        }

        return true;
    }
};

/*######
## npc_inquisitor_hallard. Quest 12321
######*/


enum eInquisitor
{
    NPC_GODFREY                 = 27577,
    SPELL_HOLY_FIRE             = 39323,

    SAY_WP_0                    = -1800014,
    SAY_WP_1                    = -1800015,
    SAY_WP_2                    = -1800016,
    SAY_WP_3                    = -1800017,
    SAY_WP_4                    = -1800018,
    SAY_WP_5                    = -1800019,
    SAY_WP_6                    = -1800020,
    SAY_WP_7                    = -1800021,
    SAY_WP_8                    = -1800022,
    SAY_WP_9                    = -1800023,
    SAY_WP_10                   = -1800024,
    SAY_WP_11                   = -1800025,
    SAY_WP_12                   = -1800026,
    SAY_WP_13                   = -1800027,
    SAY_WP_14                   = -1800028,
    SAY_WP_15                   = -1800029,
    SAY_WP_16                   = -1800030,
    SAY_WP_17                   = -1800031,
    SAY_WP_18                   = -1800032,
    SAY_WP_19                   = -1800033,
    SAY_WP_20                   = -1800034,
    SAY_WP_21                   = -1800035,
    SAY_WP_22                   = -1800036,
    SAY_WP_23                   = -1800037,
    SAY_WP_24                   = -1800038,
    SAY_WP_25                   = -1800039,
    SAY_WP_26                   = -1800040,
    SAY_WP_27                   = -1800041
};

#define QUEST_A_RIGHTEOUS_SERMON     12321

class npc_inquisitor_hallard : public CreatureScript
{
public:
    npc_inquisitor_hallard() : CreatureScript("npc_inquisitor_hallard") { }

    struct npc_inquisitor_hallardAI : public npc_escortAI
    {
        npc_inquisitor_hallardAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        bool Completed;

        void WaypointReached(uint32 i)
        {
            Player* pPlayer = GetPlayerForEscort();
            if (!pPlayer)
                return;
            Creature* Godfrey = me->FindNearestCreature(NPC_GODFREY, 50, true);
            if (!Godfrey)
                return;
            switch (i)
            {
                case 1:
                    DoScriptText(SAY_WP_1, me, Godfrey);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, Godfrey->GetGUID());
                    me->HandleEmoteCommand(5);
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;
                case 2:
                    Godfrey->HandleEmoteCommand(434);
                    DoScriptText(SAY_WP_2, me, Godfrey);
                    me->HandleEmoteCommand(15);
                    break;
                case 3:
                    DoScriptText(SAY_WP_3, me, Godfrey);
                    me->HandleEmoteCommand(1);
                    break;
                case 4:
                    DoScriptText(SAY_WP_4, Godfrey);
                    break;
                case 5:
                    DoScriptText(SAY_WP_5, Godfrey);
                    break;
                case 6:
                    DoScriptText(SAY_WP_6, Godfrey);
                    break;
                case 7:
                    DoScriptText(SAY_WP_7, me, Godfrey);
                    me->HandleEmoteCommand(1);
                    break;
                case 8:
                    DoScriptText(SAY_WP_8, me, Godfrey);
                    me->HandleEmoteCommand(16);
                    break;
                case 9:
                    DoScriptText(SAY_WP_9, me, Godfrey);
                    me->HandleEmoteCommand(5);
                    break;
                case 10:
                    DoScriptText(SAY_WP_10, me, Godfrey);
                    DoCast(Godfrey, SPELL_HOLY_FIRE);
                    break;
                case 11:
                    Godfrey->HandleEmoteCommand(434);
                    DoScriptText(SAY_WP_11, Godfrey);
                    break;
                case 12:
                    DoScriptText(SAY_WP_12, me, Godfrey);
                    DoCast(Godfrey, SPELL_HOLY_FIRE);
                    break;
                case 13:
                    DoScriptText(SAY_WP_13, me, Godfrey);
                    DoCast(Godfrey, SPELL_HOLY_FIRE);
                    break;
                case 14:
                    Godfrey->HandleEmoteCommand(434);
                    DoScriptText(SAY_WP_14, Godfrey);
                    break;
                case 15:
                    DoScriptText(SAY_WP_15, me, Godfrey);
                    DoCast(Godfrey, SPELL_HOLY_FIRE);
                    break;
                case 16:
                    DoScriptText(SAY_WP_16, me, Godfrey);
                    break;
                case 17:
                    DoScriptText(SAY_WP_17, me, Godfrey);
                    break;
                case 18:
                    DoScriptText(SAY_WP_18, Godfrey);
                    break;
                case 19:
                    DoScriptText(SAY_WP_19, me, Godfrey);
                    break;
                case 20:
                    DoScriptText(SAY_WP_20, Godfrey);
                    break;
                case 21:
                    DoScriptText(SAY_WP_21, Godfrey);
                    break;
                case 22:
                    DoScriptText(SAY_WP_22, me, Godfrey);
                    break;
                case 23:
                    DoScriptText(SAY_WP_23, Godfrey);
                    break;
                case 24:
                    DoScriptText(SAY_WP_24, Godfrey);
                    break;
                case 25:
                    DoScriptText(SAY_WP_25, me, Godfrey);
                    break;
                case 26:
                    DoScriptText(SAY_WP_26, me);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, pPlayer->GetGUID());
                    break;
                case 27:
                    DoScriptText(SAY_WP_27, me, Godfrey);
                    me->SetUInt64Value(UNIT_FIELD_TARGET, Godfrey->GetGUID());
                    Completed = true;
                    if (pPlayer)
                        pPlayer->GroupEventHappens(QUEST_A_RIGHTEOUS_SERMON, me);
                    break;
            }
        }

        void Reset()
        {
            Completed = false;
        }

        void UpdateAI(const uint32 diff)
        {
            npc_escortAI::UpdateAI(diff);
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_A_RIGHTEOUS_SERMON)
        {
            DoScriptText(SAY_WP_0, pCreature, pPlayer);
            if (npc_escortAI* pEscortAI = CAST_AI(npc_inquisitor_hallard::npc_inquisitor_hallardAI, pCreature->AI()))
            {
                pEscortAI->Start(true, false, pPlayer->GetGUID(), 0, true);
                pCreature->GetMotionMaster()->MovePoint(0, 3801.543f, -679.350f, 213.75f);
            }
        }
        return true;
    }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_inquisitor_hallardAI(creature);
    }
};


class frostcav_control: public CreatureScript
{
public:
    frostcav_control(): CreatureScript("frostcav_control") {};

    struct frostcav_controlAI : public ScriptedAI
    {
       frostcav_controlAI(Creature *c) : ScriptedAI(c) {Reset();}

       uint32 phase;
       uint32 phasetim;

       Creature* arthas;
       Creature* muradin;
       Creature* frostmourne;

       uint32 frostcav_event;
       Unit* playerr;

       void Reset()
       {
	       arthas = NULL;
    	   muradin = NULL;
	       frostmourne = NULL;
           phase = 1;
	       phasetim = 20000;
           frostcav_event = 0;
           playerr = NULL;
       };

       void Aggro(Unit* who) {};

       void UpdateAI(const uint32 diff)
       {
	       if(frostcav_event == 0) return;

    	   switch(phase)
	       {
    	   case 1:
               arthas = me->SummonCreature(27455,4817.506f,-582.1f,163.37f,5.25f,TEMPSUMMON_MANUAL_DESPAWN,104000);
	    	   muradin = me->SummonCreature(27480,4815.1f,-589.45f,162.85f,0.0f,TEMPSUMMON_MANUAL_DESPAWN,104000);
    		   frostmourne = me->SummonCreature(27880,4821.35f,-586.58f,163.76f,0.0f,TEMPSUMMON_MANUAL_DESPAWN,104000);
		       frostmourne->LoadEquipment(88001);
	    	   frostmourne->CastSpell(frostmourne,45776,false);
    		   ++phase;
		       phasetim = 2000;
	    	   break;
    	   case 3:
		       arthas->SetOrientation(5.25f);
	    	   arthas->MonsterSay("Behold, Muradin, our salvation, Frostmourne.",LANG_UNIVERSAL,0);
    		   ++phase;
		       phasetim = 4000;
	    	   break;
    	   case 5:
	    	   muradin->GetMotionMaster()->MovePoint(0, 4818.86f, -587.33f, 163.24f);
    		   ++phase;
		       phasetim = 4000;
	    	   break;
    	   case 7:
		       muradin->SetStandState(UNIT_STAND_STATE_KNEEL);
	    	   muradin->MonsterSay("Hold, lad. There\'s an inscription on the dais. It\'s a warning. It says, \"Whomsoever takes up this blade shall wield power eternal. Just as the blade rends flesh, so must power scar the spirit.\" Oh, I should\'ve known. The blade is cursed! Let\'s get the hell out of here!",LANG_UNIVERSAL,0);
    		   ++phase;
		       phasetim = 16000;
	    	   break;
    	   case 9:
               muradin->SetStandState(UNIT_STAND_STATE_STAND);
    		   ++phase;
		       phasetim = 2000;
	    	   break;
    	   case 11:
		       //muradin->GetMotionMaster()->MovePoint(0, 4819.44f, -586.73f, 163.25f);
	    	   //Muradin's orientation
    		   arthas->MonsterSay("I would gladly bear any curse to save my homeland.",LANG_UNIVERSAL,0);
		       ++phase;
		       phasetim = 1000;
	    	   break;
    	   case 13:
	    	   muradin->GetMotionMaster()->MovePoint(0, 4811.28f, -588.28f, 162.099f);
    		   ++phase;
		       phasetim = 4000;
	    	   break;
    	   case 15:
               //muradin->GetMotionMaster()->MovePoint(0, 4811.84f, -588.229f, 162.24f);
	    	   //Set right orientation (SetOrientation doesn't work?)
    		   ++phase;
		       phasetim = 2000;
	    	   break;
    	   case 17:
	    	   muradin->MonsterSay("Leave it be, Arthas. Forget this business and lead your men home.",LANG_UNIVERSAL,0);
    		   ++phase;
		       phasetim = 7000;
	    	   break;
    	   case 19:
	    	   arthas->MonsterSay("Damn the men! Nothing shall prevent me from having my revenge, old friend. Not even you.",LANG_UNIVERSAL,0);
    		   ++phase;
		       phasetim = 5000;
	    	   break;
    	   case 21:
               arthas->GetMotionMaster()->MovePoint(0, 4819.099f, -584.67f, 163.44f);
	    	   ++phase;
    		   phasetim = 3000;
		       break;
	       case 23:
    		   arthas->MonsterSay("Now, I call out to the spirits of this place. I will give anything or pay any price, if only you will help me save my people.",LANG_UNIVERSAL,0);
		       arthas->CastSpell(me,49824,false);
	    	   ++phase;
    		   phasetim = 10000;
		       break;
	       case 25:
    		   muradin->SetStandState(UNIT_STAND_STATE_DEAD);
		       arthas->LoadEquipment(88001);
	    	   frostmourne->LoadEquipment(1613);
    		   ++phase;
		       phasetim = 2000;
	    	   break;
    	   case 27:
	    	   arthas->SetUnitMovementFlags(MOVEMENTFLAG_FORWARD);
    		   arthas->SetSpeed(MOVE_RUN,1.0f);
		       arthas->SetSpeed(MOVE_WALK,1.0f);
	    	   arthas->GetMotionMaster()->MovePoint(0, 4796.64f, -586.94f, 159.55f);
    		   ++phase;
		       phasetim = 5000;
	    	   break;
    	   case 29:
               arthas->CastSpell(arthas,53048,false);
	    	   ++phase;
    		   phasetim = 2000;
		       break;
	       case 31:
    		   arthas->GetMotionMaster()->MovePoint(0, 4729.22f, -560.39f, 165.96f);
               ++phase;
		       phasetim = 2000;
	    	   break;
    	   case 33:
	    	   muradin->SetStandState(UNIT_STAND_STATE_STAND);
    		   muradin->CastSpell(muradin,43328,false);
		       ++phase;
	    	   phasetim = 3000;
    		   break;
	       case 35:
	    	   muradin->MonsterSay("O\' my head... Wh... Where am I?",LANG_UNIVERSAL,0);
    		   ++phase;
		       phasetim = 4000;
	    	   break;
    	   case 37:
		       muradin->MonsterSay("Who... Who am I?",0,0);
	    	   ++phase;
    		   phasetim = 4000;
		       break;
	       case 39:
    		   muradin->SetUnitMovementFlags(MOVEMENTFLAG_FORWARD);
		       muradin->SetSpeed(MOVE_RUN,1.0f);
	    	   muradin->GetMotionMaster()->MovePoint(0, 4796.64f, -586.94f, 159.55f);
               ++phase;
		       phasetim = 4000;
	    	   break;
    	   case 41:
		       if(muradin) muradin->SetVisible(false);
	    	   if(arthas) arthas->SetVisible(false);
    		   if(frostmourne) frostmourne->SetVisible(false);
		       //TODO: Remove muradin, arthas and frostmourne from world
	    	   frostcav_event = 0;
    		   phase = 1;
		       ((Player*)playerr)->GroupEventHappens(12478,me);
	    	   break;
    	   }

	       if (phasetim <= diff)
    	   {
	    	   ++phase;
    		   phasetim = 20000000;
	        } phasetim -= diff;
        }
    };

    CreatureAI* GetAI(Creature *_Creature) const
    {
        return new frostcav_controlAI(_Creature);
    };

    bool OnGossipHello(Player *player, Creature *_Creature)
    {
        if(((frostcav_controlAI*)_Creature->GetAI())->frostcav_event == 0)
            player->ADD_GOSSIP_ITEM(0, "I\'m ready to see Zelig\'s Vision", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

        player->PlayerTalkClass->SendGossipMenu(907,_Creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *_Creature, uint32 sender, uint32 action )
    {
        if(action == GOSSIP_ACTION_INFO_DEF+2)
        {
            ((frostcav_controlAI*)_Creature->GetAI())->frostcav_event = 1;
		    ((frostcav_controlAI*)_Creature->GetAI())->playerr = player;
		
            player->CLOSE_GOSSIP_MENU();
        }
        return true;
    }
};

void AddSC_dragonblight()
{
    new npc_alexstrasza_wr_gate;
    new npc_inquisitor_hallard;
    new frostcav_control;
}
