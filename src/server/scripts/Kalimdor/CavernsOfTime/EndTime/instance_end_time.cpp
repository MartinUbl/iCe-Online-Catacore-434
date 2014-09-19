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
#include "endtime.h"

// Time Transit Device
#define ENTRANCE           "Teleport to Entrance"                // Entrance
#define SYLVANAS           "Teleport to Ruby Dragonshrine"       // Sylvanas
#define TYRANDE            "Teleport to Emerald Dragonsshrine"   // Tyrande
#define JAINA              "Teleport to Azure Dragonshrine"      // Jaina
#define BAINE              "Teleport to Obsidian Dragonshrine"   // Baine
#define MUROZOND           "Teleport to Bronze Dragonshrine"     // Murozond

// Quests
enum Quests
{
    SYLVANAS_AND_JAINA         = 158700,
    BAINE_AND_TYRANDE          = 158701,
    SYLVANAS_AND_BAINE         = 158702,
    JAINA_AND_TYRANDE          = 158703,
    SYLVANAS_AND_TYRANDE       = 158704,
    JAINA_AND_BAINE            = 158705,
};

// Spells
enum NPC
{
    SPELL_ENTRANCE        = 102564,
    SPELL_SYLVANAS        = 102579,
    SPELL_JAINA           = 102126,
    SPELL_BAINE           = 103868,
    SPELL_TYRANDE         = 104761,
    SPELL_MUROZOND        = 104764,
};

class go_time_transit_device : public GameObjectScript
{
public:
    go_time_transit_device() : GameObjectScript("go_time_transit_device") { }

    bool OnGossipSelect(Player* pPlayer, GameObject* pGo, uint32 uiSender, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiSender)
        {
            case GOSSIP_SENDER_MAIN:    SendActionMenu(pPlayer, pGo, uiAction); break;
        }
        return true;
    }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        // Sylvanas and Jaina
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_JAINA) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        }
        // Baine and Tyrande
        if (pPlayer->GetQuestStatus(BAINE_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Sylvanas and Baine
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_BAINE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Jaina and Tyrande
        if (pPlayer->GetQuestStatus(JAINA_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        // Sylvanas and Tyrande
        if (pPlayer->GetQuestStatus(SYLVANAS_AND_TYRANDE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, SYLVANAS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, TYRANDE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
        }
        // Jaina and Baine
        if (pPlayer->GetQuestStatus(JAINA_AND_BAINE) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JAINA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, BAINE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
        }
        // Enable port to Murozond if Q for Echoes complete (resets every day)
        if ((pPlayer->GetQuestStatus(SYLVANAS_AND_JAINA) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(BAINE_AND_TYRANDE) == QUEST_STATUS_COMPLETE)
            || (pPlayer->GetQuestStatus(SYLVANAS_AND_BAINE) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(JAINA_AND_TYRANDE) == QUEST_STATUS_COMPLETE)
            || (pPlayer->GetQuestStatus(SYLVANAS_AND_TYRANDE) == QUEST_STATUS_COMPLETE) || (pPlayer->GetQuestStatus(JAINA_AND_BAINE) == QUEST_STATUS_COMPLETE))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ENTRANCE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MUROZOND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
        }

        pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pGo->GetGUID());
        return true;
    }

    void SendActionMenu(Player* pPlayer, GameObject* /*pGo*/, uint32 uiAction)
    {
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pPlayer->CastSpell(pPlayer,SPELL_ENTRANCE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                  pPlayer->CastSpell(pPlayer,SPELL_SYLVANAS,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                pPlayer->CastSpell(pPlayer,SPELL_JAINA,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4:
                pPlayer->CastSpell(pPlayer,SPELL_BAINE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5:
                pPlayer->CastSpell(pPlayer,SPELL_TYRANDE,false);
                break;
            case GOSSIP_ACTION_INFO_DEF + 6:
                pPlayer->CastSpell(pPlayer,SPELL_MUROZOND,false);
                break;
        }
    }
};

// Image of Nozdormu
static const uint32 Bosses[4] = {54123, 54445, 54431, 54544}; // Sylvanas, Jaina, Baine, Tyrande (in this order)

enum ScriptTexts
{
    // Neutral
    SAY_NEUTRAL          = -1999926, // 25959 - You must give peace to this lands if you are to face Murozond

    // Sylvanas Encounter
    SAY_SYLVANAS_1       = -1999927, // 25958 - This is where she stood, heroes, and this is where she fell. The time-lost echo of Sylvanas Windrunner will reverberate through the rotting limbs of the Dragonshrine for all eternity.

    // Baine Encounter
    SAY_BAINE_1          = -1999928, // 25956 - The undying flames are all that remain of this sacred place. I sense much anger here... a seething rage, barely held in check. Be on your guard.
    YELL_BAINE_1         =    25911, // 25911 - You! Are you the ones responsible for what has happened here...?

    // Tyrande Encounter
    SAY_TYRANDE_1        = -1999929, // 25955 - There is an unnatural darkness to this place, a perpetual midnight. Take caution, heroes, and do not stray from the light.
    TYRANDE_INTRO_1      = -1999936, // 25977 - There is nothing left for you here, nothing but death and sorrow.
    TYRANDE_INTRO_2      = -1999937, // 25978 - The darkness surrounds you, the light of Elune is your only salvation.
    TYRANDE_INTRO_3      = -1999938, // 25979 - The moonlight can bring rest to your weary souls in this forgotten place.
    TYRANDE_INTRO_4      = -1999939, // 25980 - Give yourselves to the night, Elune will guide you from this mortal prison.
    TYRANDE_INTRO_5      = -1999940, // 25981 - You have chosen a path of darkness. Mother moon, guide my hand; allow me to bring rest to these misbegotten souls.

    // Jaina Encounter
    SAY_JAINA_1          = -1999930, // 25957 - This is all that is left of the Blue Dragonshrine. A great battle shattered the timeways leading out of this forsaken place. You must reconstruct the fragments strewn across the ground and defeat the trapped spirit to proceed.
    JAINA_INTRO          = -1999941, // 25920 - I don't know who you are, but I'll defend this shrine with my life. Leave, now, before we come to blows.

    // Murozond Encounter
    YELL_MUROZOND_1      =    25934, // 25934 - The "End Time," I once called this place, this strand. I had not seen, by then; I did not know. You hope to... what? Stop me, here? Change the fate I worked so tirelessly to weave?
    YELL_MUROZOND_2      =    25935, // 25935 - You crawl unwitting, like a blind, writhing worm, towards endless madness and despair. I have witnessed the true End Time. This? This is a blessing you simply cannot comprehend.
};

class npc_image_of_nozdormu : public CreatureScript
{
public:
    npc_image_of_nozdormu() : CreatureScript("npc_image_of_nozdormu") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_image_of_nozdormuAI (pCreature);
    }

    struct npc_image_of_nozdormuAI : public ScriptedAI
    {
        npc_image_of_nozdormuAI(Creature *c) : ScriptedAI(c) {}

        uint32 Check_Timer;
        uint32 Say_Neutral_Check;
        uint32 Say_Next;
        uint32 Baine_Welcome_Yell_Timer;
        bool Sylvanas_Say;
        bool Jaina_Say;
        bool Tyrande_Say;
        bool Baine_Say;
        bool Image_Of_Nozdormu_Say;
        bool Baine_Welcome_Yell;

        void Reset()
        {
            me->CastSpell(me, 102602, true); // Visual aura

            Check_Timer = 10000;
            Say_Neutral_Check = 10000;

            Sylvanas_Say = false;
            Jaina_Say = false;
            Tyrande_Say = false;
            Baine_Say = false;
            Image_Of_Nozdormu_Say = false;
            Baine_Welcome_Yell = false;
        }

        void EnterCombat(Unit * /*who*/) { }

        void UpdateAI(const uint32 diff) 
        {
            if (Check_Timer <= diff)
            {
                float distance;
                int count;
                count = 0;
                Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
                if (!playerList.isEmpty())
                    for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                        if (Player* pPlayer = i->getSource())
                        {
                            distance = me->GetExactDist2d(pPlayer);
                            if (distance<50)
                                count = count+1;
                        }

                if (count >= 4) // Say only when at least 4 memebrs of the group are near Image of Nozdormu, so most of them can enjoy this speech :D
                {
                    Creature * boss = NULL;
                    Creature * new_boss = NULL;
                    for (int i = 0; i<4; i++)
                    {
                        new_boss = me->FindNearestCreature(Bosses[i], 250.0f, true);
                        if (new_boss)
                            boss = new_boss;
                    }
                    
                    if (boss)
                    {
                        int entry = boss->GetEntry();
                        switch(entry)
                        {
                            // Sylvanas
                            case(54123):
                                {
                                    if (Sylvanas_Say == false)
                                    {
                                        DoScriptText(SAY_SYLVANAS_1,me);
                                        me->SendPlaySound(25958, true);
                                        Sylvanas_Say = true;
                                        Say_Next = 17000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            // Jaina
                            case(54445):
                                {
                                    if (Jaina_Say == false)
                                    {
                                        DoScriptText(SAY_JAINA_1,me);
                                        me->SendPlaySound(25957, true);
                                        Jaina_Say = true;
                                        Say_Next = 19000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            // Baine
                            case(54431):
                                {
                                    if (Baine_Say == false)
                                    {
                                        DoScriptText(SAY_BAINE_1,me);
                                        me->SendPlaySound(25956, true);
                                        Baine_Say = true;
                                        Say_Next = 17000;
                                        Image_Of_Nozdormu_Say = true;
                                        Baine_Welcome_Yell_Timer = 32000;
                                        Baine_Welcome_Yell = true;
                                    }
                                }
                                break;
                            // Tyrande
                            case(54544):
                                {
                                    if (Tyrande_Say == false)
                                    {
                                        DoScriptText(SAY_TYRANDE_1,me);
                                        me->SendPlaySound(25955, true);
                                        Tyrande_Say = true;
                                        Say_Next = 14000;
                                        Image_Of_Nozdormu_Say = true;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    Check_Timer = 10000;
                } else Check_Timer -= diff;

                if (Image_Of_Nozdormu_Say == true)
                {
                    if (Say_Next <= diff)
                    {
                        DoScriptText(SAY_NEUTRAL,me);
                        me->SendPlaySound(25959, true);
                        Image_Of_Nozdormu_Say = false;
                    }
                    else Say_Next -= diff;
                }

                if (Baine_Welcome_Yell == true)
                {
                    if (Baine_Welcome_Yell_Timer <= diff)
                    {
                        Creature * baine = me->FindNearestCreature(54431, 300.0f, true);
                        if (baine)
                        {
                            baine->MonsterYell("You! Are you the ones responsible for what has happened here...?", LANG_UNIVERSAL, 0);
                            baine->SendPlaySound(25911, false);
                        }
                        Baine_Welcome_Yell = false;
                    }
                    else Baine_Welcome_Yell_Timer -= diff;
                }
            }

    };
};

//////////////////////////////////////////////////////////////
////////////////        TRASH AI            //////////////////
//////////////////////////////////////////////////////////////

/////////////////////////
// Bronze Dragonshrine //
/////////////////////////
enum Creatures
{
    INFINITE_SUPPRESSOR           = 54920,
    INFINITE_WARDEN               = 54923,
    MUROZOND_BOSS                 = 54432,
};
enum BronzeDragonshrineSpells
{
    ARCANE_WAVE                   = 102601,
    TEMPORAL_VORTEX               = 102600,

    VOID_SHIELD                   = 102599,
    VOID_STRIKE                   = 102598,
};

// Infinite Suppressor
class npc_infinite_suppressor : public CreatureScript
{
public:
    npc_infinite_suppressor() : CreatureScript("npc_infinite_suppressor") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_infinite_suppressorAI (pCreature);
    }

    struct npc_infinite_suppressorAI : public ScriptedAI
    {
        npc_infinite_suppressorAI(Creature *c) : ScriptedAI(c) {}

        uint32 Arcane_Wave;
        uint32 Temporal_Vortex;

        void Reset()
        {
            Arcane_Wave = 2000;
            Temporal_Vortex = 15000+urand(0,10000);
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

            if (Arcane_Wave <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
                if (target)
                    me->CastSpell(target, ARCANE_WAVE, false);
                Arcane_Wave = 4000;
            }
            else Arcane_Wave -= diff;

            if (Temporal_Vortex <= diff)
            {
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true);
                if (target)
                    me->CastSpell(target, TEMPORAL_VORTEX, false);
                Temporal_Vortex = 15000;
            }
            else Temporal_Vortex -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Infinite Warden
class npc_infinite_warden : public CreatureScript
{
public:
    npc_infinite_warden() : CreatureScript("npc_infinite_warden") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_infinite_wardenAI (pCreature);
    }

    struct npc_infinite_wardenAI : public ScriptedAI
    {
        npc_infinite_wardenAI(Creature *c) : ScriptedAI(c) { }

        uint32 Void_Shield;
        uint32 Void_Strike;

        void Reset()
        {
            me->RemoveAura(VOID_SHIELD);

            Void_Shield = 5000;
            Void_Strike = 10000+urand(0,7000);
        }

        void JustDied(Unit * /*who*/)
        {
            int count;
            count = 0;
            std::list<Creature*> Infinite_Wardens;
            GetCreatureListWithEntryInGrid(Infinite_Wardens, me, INFINITE_WARDEN, 250.0f);
            for (std::list<Creature*>::const_iterator itr = Infinite_Wardens.begin(); itr != Infinite_Wardens.end(); ++itr)
                if ((*itr) && (*itr)->IsAlive())
                    count = count + 1;

            if (count == 2)
            {
                Creature * murozond_boss = me->FindNearestCreature(MUROZOND_BOSS, 250.0, true);
                if (murozond_boss)
                {
                    murozond_boss->MonsterYell("The \"End Time\", I once called this place, this strand. I had not seen, by then; I did not know. You hope to... what? Stop me, here? Change the fate I worked so tirelessly to weave?", LANG_UNIVERSAL, 0);
                    murozond_boss->SendPlaySound(25934, false);
                }
            }

            if (count == 0)
            {
                Creature * murozond_boss = me->FindNearestCreature(MUROZOND_BOSS, 250.0, true);
                if (murozond_boss)
                {
                    murozond_boss->MonsterYell("You crawl unwitting, like a blind, writhing worm, towards endless madness and despair. I have witnessed the true End Time. This? This is a blessing you simply cannot comprehend.", LANG_UNIVERSAL, 0);
                    murozond_boss->SendPlaySound(25935, false);
                }
            }
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim()) 
                return;

            if (Void_Shield <= diff)
            {
                me->CastSpell(me, VOID_SHIELD, true);
                Void_Shield = 16000;
            }
            else Void_Shield -= diff;

            if (Void_Strike <= diff)
            {
                Unit * target = me->GetVictim();
                if (target)
                    me->CastSpell(target, VOID_STRIKE, false);
                Void_Strike = 10000+urand(0,3000);
            }
            else Void_Strike -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

///////////////////////////
// Obsidian Dragonshrine //
///////////////////////////
enum CreaturesObsidian
{
    TIME_TWISTED_SEER             = 54553,
    TIME_TWISTED_BREAKER          = 54552,
    TIME_TWISTED_DRAKE            = 54543,

    RUPTURED_GROUND_NPC           = 54566,
    CALL_FLAMES_NPC               = 54585,
};

enum ObsidianDragonshrineSpells
{
    SEAR_FLESH                    = 102158,
    CALL_FLAMES                   = 102156,
    UNDYING_FLAME                 = 105238,

    BREAK_ARMOR                   = 102132,
    RUPTURE_GROUND                = 102124,
    RUPTURED_GROUND               = 102128,

    ENRAGE                        = 102134,
    FLAME_BREATH                  = 102135,
};

// Time-Twisted Seer
class npc_time_twisted_seer : public CreatureScript
{
public:
    npc_time_twisted_seer() : CreatureScript("npc_time_twisted_seer") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_seerAI (pCreature);
    }

    struct npc_time_twisted_seerAI : public ScriptedAI
    {
        npc_time_twisted_seerAI(Creature *c) : ScriptedAI(c) {}

        uint32 Sear_Flesh;
        uint32 Call_Flames;

        void Reset() 
        {
            Sear_Flesh = 8000;
            Call_Flames = 15000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Sear_Flesh <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                    if (target)
                        me->CastSpell(target, SEAR_FLESH, false);
                Sear_Flesh = 10000;
            }
            else Sear_Flesh -= diff;

            if (Call_Flames <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, CALL_FLAMES, false);
                Call_Flames = 15000;
            }
            else Call_Flames -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Time-Twisted Breaker
class npc_time_twisted_breaker : public CreatureScript
{
public:
    npc_time_twisted_breaker() : CreatureScript("npc_time_twisted_breaker") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_breakerAI (pCreature);
    }

    struct npc_time_twisted_breakerAI : public ScriptedAI
    {
        npc_time_twisted_breakerAI(Creature *c) : ScriptedAI(c) {}

        uint32 Break_Armor;
        uint32 Rupture_Ground;

        void Reset() 
        {
            Break_Armor = 5000;
            Rupture_Ground = 10000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Break_Armor <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, BREAK_ARMOR, false);
                Break_Armor = 15000;
            }
            else Break_Armor -= diff;

            if (Rupture_Ground <= diff)
            {
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
                me->CastSpell(me, RUPTURE_GROUND, false);
                Rupture_Ground = 20000;
            }
            else Rupture_Ground -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Time-Twisted Drake
class npc_time_twisted_drake : public CreatureScript
{
public:
    npc_time_twisted_drake() : CreatureScript("npc_time_twisted_drake") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_time_twisted_drakeAI (pCreature);
    }

    struct npc_time_twisted_drakeAI : public ScriptedAI
    {
        npc_time_twisted_drakeAI(Creature *c) : ScriptedAI(c) {}

        uint32 Flame_Breath;
        uint32 Enrage;

        void Reset() 
        {
            Flame_Breath = 10000;
            Enrage = 15000;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (!UpdateVictim())
                return;

            if (Enrage <= diff)
            {
                me->CastSpell(me, ENRAGE, false);
                Enrage = 30000;
            }
            else Enrage -= diff;

            if (Flame_Breath <= diff)
            {
                me->CastSpell(me, FLAME_BREATH, false);
                Flame_Breath = 14000;
            }
            else Flame_Breath -= diff;

            DoMeleeAttackIfReady();
        }

    };
};

// Ruptured Ground
class npc_ruptured_ground : public CreatureScript
{
public:
    npc_ruptured_ground() : CreatureScript("npc_ruptured_ground") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ruptured_groundAI (pCreature);
    }

    struct npc_ruptured_groundAI : public ScriptedAI
    {
        npc_ruptured_groundAI(Creature *c) : ScriptedAI(c) {}

        void Reset() 
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, RUPTURED_GROUND, false);
        }

    };
};

// Call Flames
class npc_call_flames : public CreatureScript
{
public:
    npc_call_flames() : CreatureScript("npc_call_flames") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_call_flamesAI (pCreature);
    }

    struct npc_call_flamesAI : public ScriptedAI
    {
        npc_call_flamesAI(Creature *c) : ScriptedAI(c) {}

        uint32 Undying_Flame;
        int count;

        void Reset() 
        {
            Undying_Flame = 500;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            count = 0;
        }

        void UpdateAI(const uint32 diff) 
        {
            if (Undying_Flame <= diff)
            {
                me->CastSpell(me, UNDYING_FLAME, false);
                count = count + 1;

                if (count == 4)
                    me->Kill(me); 
                Undying_Flame = 1000;
            } 
            else Undying_Flame -= diff;
        }
    };
};

void AddSC_instance_end_time()
{
    new go_time_transit_device();

    new npc_image_of_nozdormu();

    new npc_infinite_suppressor();
    new npc_infinite_warden();
    new npc_time_twisted_seer();
    new npc_time_twisted_breaker();
    new npc_time_twisted_drake();
    new npc_ruptured_ground();
    new npc_call_flames();
}