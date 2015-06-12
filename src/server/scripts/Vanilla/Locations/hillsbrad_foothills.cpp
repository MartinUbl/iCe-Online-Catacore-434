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

#include "ScriptPCH.h"

enum GossipTexts
{
    GOSSIP_TEXT_DUMASS_1 = 51000, // HI! I'M NEW! BIG WHITE LIGHT CREATURE WITH WINGS MADE ME ALIVE! I SERVE THE BANSHEE QUEEN! YAY! <NEXTLINE> HELP! <NEXTLINE> HI! 
    GOSSIP_TEXT_DUMASS_2 = 51001, // OK! THANKS! I'LL JUST WAIT HERE WITH YOU! THANKS! OK!
    GOSSIP_TEXT_ORKUS_1 = 51002, // I have seen the fall of the Lich King. Creations of the titans have fallen before my mighty axe. When called upon, I alone spearheaded a victory for the Argent Crusade against the beasts of Northrend. Now, I come for the ultimate challenge. What does Hillsbrad Foothills have to offer Kingslayer Orkus?
    GOSSIP_TEXT_ORKUS_2 = 51003, // Bloodthirsty you say? Is there any risk of death or dismemberment to me?
    GOSSIP_TEXT_ORKUS_3 = 51004, // Then Orkus WILL DO IT! YES!
    GOSSIP_TEXT_JOHNY_1 = 51005, // Look at me, peasant. Heirlooms cover my body from head to toe, gifted to me by the greatest heroes Azeroth has ever known. <NEXTLINE> <NEXTLINE> Now look at yourself. <NEXTLINE> <NEXTLINE> Quickly, look back at me. <NEXTLINE> <NEXTLINE> Yes, this IS horse made of STARS. <NEXTLINE> <NEXTLINE> What pointless series of tasks befitting a mentally deficient orc have you prepared for me? 
    GOSSIP_TEXT_JOHNY_2 = 51006, // Fine, fine, what else?
    GOSSIP_TEXT_JOHNY_3 = 51007 // That's all? One quest? Surely you jest. Are there not bear asses to collect? Perhaps a rare flower that I could pick from which you will make some mildly hallucinogenic tonic which you will then drink, resulting in visions of a great apocalypse? Perhaps the local populace of mildly annoying, ill-tempered gophers are acting up and need to be brought to justice? No? Nothing? 
};

enum Spells
{
    SPELL_QUEST_GIVER_BUFF = 88476,
    SPELL_FROST_WYRM = 67336,
    SPELL_CELESTIAL_STEED = 75614,
    SPELL_ROOT = 42716,
    SPELL_DUMMY_QUESTCOMPLETE = 54694
};

enum IDs
{
    QUEST_ID = 28096,
    NPC_QUEST_GIVER = 2215,
    NPC_DUMASS = 47444,
    NPC_ORKUS = 47443,
    NPC_JOHNY = 47442,
    NPC_SKELETAL_STEED = 47445
};

enum SteedEvents
{
    EVENT_GIVER_SAY_1 = 1,
    EVENT_SPAWN_DUMASS,
    EVENT_GIVER_SAY_2,
    EVENT_GIVER_SAY_3,
    EVENT_SPAWN_ORKUS
};


enum DumassEvents
{
    EVENT_DUMASS_MOVE_1 = 1,
    EVENT_DUMASS_MOVE_2,
    EVENT_DUMASS_MOVE_3,
    EVENT_DUMASS_WAVE,
    EVENT_DUMASS_HELLO,
    EVENT_DUMASS_SAY,
    EVENT_DUMASS_MOVE_4,
    EVENT_DUMASS_MOVE_5
};

enum OrkusEvents
{
    EVENT_ORKUS_YELL = 1,
    EVENT_ORKUS_MOVE_1,
    EVENT_ORKUS_MOCE_2,
    EVENT_ORKUS_MOVE_3,
    EVENT_ORKUS_SAY_1,
    EVENT_ORKUS_SAY_2,
    EVENT_ORKUS_SETGOSSIP,
    EVENT_ORKUS_SAY_3,
    EVENT_ORKUS_MOVE_4
};

enum JohnyEvents
{
    EVENT_JOHNY_MOVE_1 = 1,
    EVENT_JOHNY_MOVE_2,
    EVENT_JOHNY_MOVE_3,
    EVENT_JOHNY_MOVE_4,
    EVENT_JOHNY_MOVE_5,
    EVENT_JOHNY_SAY_1,
    EVENT_JOHNY_SETGOSSIP,
    EVENT_JOHNY_SAY_2,
    EVENT_JOHNY_MOVE_6,
    EVENT_JOHNY_TURN,
    EVENT_JOHNY_MOVE_7,
    EVENT_JOHNY_MOVE_8
};

static bool wttm_questCapableSpeak(Player* player)
{
    if (!player || !player->GetVehicle() || !player->GetVehicleBase() || player->GetVehicleBase()->GetEntry() != NPC_SKELETAL_STEED)
        return false;

    return true;
}

class custom_skeletal_steed : public CreatureScript
{
public:
    custom_skeletal_steed() : CreatureScript("custom_skeletal_steed_script") {}

    struct skeletal_steedAI : public VehicleAI
    {
        Player* plPassenger;

        skeletal_steedAI(Creature* creature) : VehicleAI(creature) {}

        void Reset()
        {
            npc_questGiver = nullptr;

            DoCast(me, SPELL_ROOT, true);

            plPassenger = nullptr;
        }

        void PassengerBoarded(Unit* player, int8 /*seat*/, bool apply) override
        {
            if (player->GetTypeId() != TYPEID_PLAYER)
                return;

            if (apply)
            {
                if (player->ToPlayer()->GetQuestStatus(QUEST_ID) != QUEST_STATUS_INCOMPLETE)
                {
                    player->ToPlayer()->ExitVehicle();
                    return;
                }

                DoCast(me, SPELL_ROOT, true);
                me->SetRooted(true);

                plPassenger = player->ToPlayer();

                npc_questGiver = nullptr;
                npc_questGiver = me->FindNearestCreature(NPC_QUEST_GIVER, 30, true);

                if (!npc_questGiver)
                    return;

                DoCast(player, SPELL_QUEST_GIVER_BUFF);
                _events.ScheduleEvent(EVENT_GIVER_SAY_1, 1000);
            }
            else
            {
                plPassenger = nullptr;

                player->RemoveAura(SPELL_QUEST_GIVER_BUFF);

                if(Creature* dumass = me->FindNearestCreature(NPC_DUMASS, 100, true))
                    dumass->DespawnOrUnsummon();

                if(Creature* orkus = me->FindNearestCreature(NPC_DUMASS, 100, true))
                    orkus->DespawnOrUnsummon();

                if (Creature* johny = me->FindNearestCreature(NPC_DUMASS, 100, true))
                    johny->DespawnOrUnsummon();

                _events.Reset();
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            _events.Update(diff);

            if (!plPassenger || !npc_questGiver)
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_GIVER_SAY_1:
                {
                    std::stringstream ss;
                    ss << "Stand tall and proud, " << plPassenger->GetName() << ". Don't let any of these scrubs give you any trouble. Show them who's boss!";
                    npc_questGiver->MonsterSay(ss.str().c_str(), LANG_UNIVERSAL, 0);
                    _events.ScheduleEvent(EVENT_SPAWN_DUMASS, 3000);
                    break;
                }
                case EVENT_SPAWN_DUMASS:
                    me->SummonCreature(NPC_DUMASS, -580.245f, 456.097f, 81.749f, 3.21f, TEMPSUMMON_TIMED_DESPAWN, 180000);
                    _events.ScheduleEvent(EVENT_GIVER_SAY_2, 700);
                    break;

                case EVENT_GIVER_SAY_2:
                {
                    std::stringstream ss;
                    ss << "I see one coming now! Looks like a real winner. Keep it cool, " << plPassenger->GetName() << ". Keep it cool.";
                    npc_questGiver->MonsterSay(ss.str().c_str(), LANG_UNIVERSAL, 0);
                    break;
                }
                case EVENT_GIVER_SAY_3:
                    npc_questGiver->MonsterSay("Oh great... not this one again. I know this orc. He's got no business being here, but since he's a bottom-feeding pansy he likes to hang around here and prey on the helpless. Don't let him bully you.", LANG_UNIVERSAL, 0);
                    _events.ScheduleEvent(EVENT_SPAWN_ORKUS, 500);
                    break;

                case EVENT_SPAWN_ORKUS:
                    me->SummonCreature(NPC_ORKUS, -584.222f, 467.557f, 84.054f, 4.73f, TEMPSUMMON_TIMED_DESPAWN, 180000);
                    break;
                }
            }

        }

        void GetReadyToSpawnOrkus()
        {
            _events.ScheduleEvent(EVENT_GIVER_SAY_3, 3000);
        }

        void GetReadyToSpawnJohny()
        {
            if (!plPassenger || !npc_questGiver)
                return;

            std::stringstream ss;
            ss << "Ah, crap. You're on your own with this one, " << plPassenger->GetName() << ".";
            npc_questGiver->MonsterSay(ss.str().c_str(), LANG_UNIVERSAL, 0);
            me->SummonCreature(NPC_JOHNY, -582.675f, 470.419f, 82.483f, 4.77f, TEMPSUMMON_TIMED_DESPAWN, 180000);
        }

    private:
        EventMap _events;
        Creature* npc_questGiver;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new skeletal_steedAI(creature);
    }
};

class custom_dumass : public CreatureScript
{
public:
    custom_dumass() : CreatureScript("custom_dumass_script") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->GetQuestStatus(QUEST_ID) != QUEST_STATUS_INCOMPLETE)
            return false;

        if (!wttm_questCapableSpeak(player))
            return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Maybe you should go take a nap or something. I don't know if I have any work for you.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(GOSSIP_TEXT_DUMASS_1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 actions)
    {
        player->PlayerTalkClass->ClearMenus();
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        switch (actions)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Wait a minute. It looks like something just came up. Yes, right here on this sheet of paper. You need to head southeast to to the Azurelode Mine and report to Captain Keyton. Southeast is that way *you point southeast*.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_DUMASS_2, creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF + 2:

            player->CLOSE_GOSSIP_MENU();
            (CAST_AI(custom_dumass::custom_dumassAI, creature->AI()))->Pokracuj();
            (CAST_AI(custom_dumass::custom_dumassAI, creature->AI()))->Set_pPlayer(player);
            break;
        }

        return true;
    }

    struct custom_dumassAI : public ScriptedAI
    {
        custom_dumassAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.ScheduleEvent(EVENT_DUMASS_MOVE_1, 1000);
        }

        void IsSummonedBy(Unit* owner) override
        {
            if (owner->GetTypeId() != TYPEID_UNIT)
                return;

            myOwner = owner;
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
            case 1:
                _events.ScheduleEvent(EVENT_DUMASS_MOVE_2, 2500);
                break;

            case 2:
                me->MonsterSay("HI! HI! HELP!", LANG_UNIVERSAL, 0);
                me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                _events.ScheduleEvent(EVENT_DUMASS_MOVE_3, 4800);
                break;

            case 3:
                _events.ScheduleEvent(EVENT_DUMASS_WAVE, 800);
                break;

            case 4:
                _events.ScheduleEvent(EVENT_DUMASS_MOVE_5, 1);
                break;

            case 5:
                (CAST_AI(custom_skeletal_steed::skeletal_steedAI, myOwner->GetAI()))->GetReadyToSpawnOrkus();
                me->DespawnOrUnsummon();
                break;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_DUMASS_MOVE_1:
                    me->GetMotionMaster()->MovePoint(1, -588.045f, 457.293f, 81.984f);
                    break;

                case EVENT_DUMASS_MOVE_2:
                    me->GetMotionMaster()->MovePoint(2, -589.775f, 429.107f, 79.474f);
                    break;

                case EVENT_DUMASS_MOVE_3:
                    me->GetMotionMaster()->MovePoint(3, -577.511f, 424.056f, 79.893f);
                    break;

                case EVENT_DUMASS_WAVE:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                    _events.ScheduleEvent(EVENT_DUMASS_HELLO, 800);
                    break;

                case EVENT_DUMASS_HELLO:
                    me->MonsterSay("HI! HI! HELP!", LANG_UNIVERSAL, 0);
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;

                case EVENT_DUMASS_SAY:
                    me->MonsterSay("NORTH! GOT IT! THANKS! BYE! THANKS!", LANG_UNIVERSAL, 0);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE);
                    _events.ScheduleEvent(EVENT_DUMASS_MOVE_4, 3000);
                    break;

                case EVENT_DUMASS_MOVE_4:
                    me->GetMotionMaster()->MovePoint(4, -583.496f, 415.646f, 79.008f);
                    break;

                case EVENT_DUMASS_MOVE_5:
                    if(Creature* npc_questGiver = me->FindNearestCreature(NPC_QUEST_GIVER, 30, true))
                        npc_questGiver->MonsterSay("These new Forsaken tend to be a little... um... stupid. It usually takes awhile for them to acclimate.", LANG_UNIVERSAL, 0);

                    if (pPlayer)
                        pPlayer->CastSpell(me, SPELL_DUMMY_QUESTCOMPLETE, true);

                    me->GetMotionMaster()->MovePoint(5, -586.961f, 381.282f, 76.259f);
                    break;
                }
            }

        }

        void Pokracuj()
        {
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            _events.ScheduleEvent(EVENT_DUMASS_SAY, 2000);
        }

        void Set_pPlayer(Player* p_player)
        {
            pPlayer = p_player;
        }

    private:
        EventMap _events;
        Unit* myOwner;
        Player* pPlayer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new custom_dumassAI(creature);
    }
};

class custom_orkus : public CreatureScript
{
public:
    custom_orkus() : CreatureScript("custom_orkus_script") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_ID) != QUEST_STATUS_INCOMPLETE)
            return false;

        if (!wttm_questCapableSpeak(player))
            return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Um... Apparently there are bloodthirsty worgen running rampant in the south. Maybe you could help with them?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(GOSSIP_TEXT_ORKUS_1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 actions) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        switch (actions)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "None.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_ORKUS_2, creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF + 2:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Right, thanks. Just go ahead and head south. Far south. Probably off the coast.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_ORKUS_3, creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF + 3:
            player->CLOSE_GOSSIP_MENU();
            (CAST_AI(custom_orkus::custom_orkusAI, creature->AI()))->Pokracuj();
            //player->KilledMonsterCredit(creature->GetEntry(), creature->GetGUID());
            player->CastSpell(creature, SPELL_DUMMY_QUESTCOMPLETE, true);
            break;
        }

        return true;
    }

    struct custom_orkusAI : ScriptedAI
    {
        custom_orkusAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFlying(true);
            me->SetUnitMovementFlags(MOVEMENTFLAG_FLYING);

            DoCast(me, SPELL_FROST_WYRM, true);
            _events.ScheduleEvent(EVENT_ORKUS_YELL, 500);
        }

        void IsSummonedBy(Unit* owner) override
        {
            if (owner->GetTypeId() != TYPEID_UNIT)
                return;

            myOwner = owner;
        }


        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
            case 1:
                _events.ScheduleEvent(EVENT_ORKUS_MOCE_2, 0);
                break;

            case 2:
                _events.ScheduleEvent(EVENT_ORKUS_MOVE_3, 0);
                break;

            case 3:
                _events.ScheduleEvent(EVENT_ORKUS_SAY_1, 2000);
                break;

            case 4:
                (CAST_AI(custom_skeletal_steed::skeletal_steedAI, myOwner->GetAI()))->GetReadyToSpawnJohny();
                me->DespawnOrUnsummon();
                break;
            }
        }


        void UpdateAI(const uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_ORKUS_YELL:
                    me->MonsterYell("CAN YOU SMELL WHAT THE LOK'TAR IS COOKIN'?", LANG_UNIVERSAL, 0);
                    _events.ScheduleEvent(EVENT_ORKUS_MOVE_1, 500);
                    break;

                case EVENT_ORKUS_MOVE_1:
                    me->GetMotionMaster()->MovePoint(1, -584.318f, 435.387f, 82.237f);
                    break;

                case EVENT_ORKUS_MOCE_2:
                    me->GetMotionMaster()->MovePoint(2, -581.893f, 426.978f, 80.915f);
                    break;

                case EVENT_ORKUS_MOVE_3:
                    me->GetMotionMaster()->MovePoint(3, -579.539f, 425.550f, 79.755f);
                    me->SetFlying(false);
                    break;

                case EVENT_ORKUS_SAY_1:
                    me->MonsterSay("HAH! Looks like you're running out of idiots to put atop this horse, Darthalia. This is the puniest one yet.", LANG_UNIVERSAL, 0);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR); // mùže být špatné emote
                    _events.ScheduleEvent(EVENT_ORKUS_SAY_2, 5000);
                    break;

                case EVENT_ORKUS_SAY_2:
                    me->MonsterSay("What have you got for me today, weakling? Point me to where the Alliance hide and I shall DOMINATE THEM!", LANG_UNIVERSAL, 0);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER); // mùže být špatné emote
                    _events.ScheduleEvent(EVENT_ORKUS_SETGOSSIP, 2000);
                    break;

                case EVENT_ORKUS_SETGOSSIP:
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;

                case EVENT_ORKUS_SAY_3:
                    me->MonsterSay("Yes, cowardly quest giver, sit atop your pale horse while Orkus brings glory to the Horde! I shall return with a thousand skulls!", LANG_UNIVERSAL, 0);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_TALK); // mùže být špatné emote
                    _events.ScheduleEvent(EVENT_ORKUS_MOVE_4, 3200);
                    break;

                case EVENT_ORKUS_MOVE_4:
                    me->SetFlying(true);
                    me->GetMotionMaster()->MovePoint(4, -589.408f, 384.678f, 94.005f);

                    if (Creature* npc_questGiver = me->FindNearestCreature(NPC_QUEST_GIVER, 30, true))
                        npc_questGiver->MonsterSay("Here's to hoping he never returns. Maybe he'll drown?", LANG_UNIVERSAL, 0);
                    break;
                }
            }

        }

        void Pokracuj()
        {
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            _events.ScheduleEvent(EVENT_ORKUS_SAY_3, 500);
        }

    private:
        EventMap _events;
        Unit* myOwner;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new custom_orkusAI(creature);
    }
};

class custom_johny : public CreatureScript
{
public:
    custom_johny() : CreatureScript("custom_johny_script") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (player->GetQuestStatus(QUEST_ID) != QUEST_STATUS_INCOMPLETE)
            return false;

        if (!wttm_questCapableSpeak(player))
            return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Well, we are having some problems at the Sludge Fields, located northeast of here. Warden Stillwater could use your help.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(GOSSIP_TEXT_JOHNY_1, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 actions) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (sender != GOSSIP_SENDER_MAIN)
            return false;

        switch (actions)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "That's all.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_JOHNY_2, creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF + 2:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "THAT'S ALL, JOHNNY AWESOME. TAKE IT OR LEAVE IT!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            player->SEND_GOSSIP_MENU(GOSSIP_TEXT_JOHNY_3, creature->GetGUID());
            break;

        case GOSSIP_ACTION_INFO_DEF + 3:
            player->CLOSE_GOSSIP_MENU();
            (CAST_AI(custom_johny::custom_johnyAI, creature->AI()))->Pokracuj();
            (CAST_AI(custom_johny::custom_johnyAI, creature->AI()))->Set_pPlayer(player);
            break;
        }
        return true;
    }

    struct custom_johnyAI : ScriptedAI
    {
        custom_johnyAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            DoCast(me, SPELL_CELESTIAL_STEED, true);
            me->SetSpeed(MOVE_RUN, 1.0f, true);
            _events.ScheduleEvent(EVENT_JOHNY_MOVE_1, 0);
        }

        void IsSummonedBy(Unit* owner) override
        { 
            if (owner->GetTypeId() != TYPEID_UNIT)
                return;

            myOwner = owner;
        }


        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
            case 1:
                _events.ScheduleEvent(EVENT_JOHNY_MOVE_2, 0);
                break;

            case 2:
                _events.ScheduleEvent(EVENT_JOHNY_MOVE_3, 0);
                break;

            case 3:
                _events.ScheduleEvent(EVENT_JOHNY_MOVE_4, 0);
                break;

            case 4:
                _events.ScheduleEvent(EVENT_JOHNY_MOVE_5, 0);
                break;

            case 5:
                _events.ScheduleEvent(EVENT_JOHNY_SAY_1, 1300);
                break;

            case 6:
                _events.ScheduleEvent(EVENT_JOHNY_TURN, 800);
                break;

            case 7:
                if(pPlayer)
                {
                    pPlayer->RemoveAura(SPELL_QUEST_GIVER_BUFF);
                    pPlayer->ExitVehicle();
                }
                _events.ScheduleEvent(EVENT_JOHNY_MOVE_8, 0);
                break;

            case 8:
                me->DespawnOrUnsummon();
                break;
            }
        }


        void UpdateAI(const uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_JOHNY_MOVE_1:
                    me->GetMotionMaster()->MovePoint(1, -579.367f, 443.789f, 81.116f);
                    break;

                case EVENT_JOHNY_MOVE_2:
                    me->GetMotionMaster()->MovePoint(2, -581.187f, 430.623f, 80.048f);
                    break;

                case EVENT_JOHNY_MOVE_3:
                    me->GetMotionMaster()->MovePoint(3, -580.844f, 428.340f, 79.907f);
                    break;

                case EVENT_JOHNY_MOVE_4:
                    me->GetMotionMaster()->MovePoint(4, -579.466f, 425.973f, 79.791f);
                    break;

                case EVENT_JOHNY_MOVE_5:
                    me->GetMotionMaster()->MovePoint(5, -578.362f, 425.177f, 79.843f);
                    break;

                case EVENT_JOHNY_SAY_1:
                    me->MonsterSay("Johnny Awesome has arrived, philistine. Present me with your menial tasks so that I may complete them with only mild enthusiasm and most likely a complete disregard for any directions that you provide that are more complicated than what my map is able to display.", LANG_UNIVERSAL, 0);
                    _events.ScheduleEvent(EVENT_JOHNY_SETGOSSIP, 6000);
                    break;

                case EVENT_JOHNY_SETGOSSIP:
                    me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    break;

                case EVENT_JOHNY_SAY_2:
                    me->MonsterSay("I will do this ONE thing that you ask of me, quest giver. Pray I find more menial tasks to accomplish or you will be hearing from me again and I assure you that my commentary on forums of public opinion will be most unkind.", LANG_UNIVERSAL, 0);
                    _events.ScheduleEvent(EVENT_JOHNY_MOVE_6, 5000);
                    break;

                case EVENT_JOHNY_MOVE_6:
                    me->GetMotionMaster()->MovePoint(6, -582.511f, 415.179f, 78.971f);
                    break;

                case EVENT_JOHNY_TURN:
                    me->SetFacingTo(0.86f);
                    _events.ScheduleEvent(EVENT_JOHNY_MOVE_7, 2500);
                    break;

                case EVENT_JOHNY_MOVE_7:
                    if (pPlayer)
                        pPlayer->CastSpell(me, SPELL_DUMMY_QUESTCOMPLETE, true);
                    me->GetMotionMaster()->MovePoint(7, -584.784f, 405.342f, 78.210f);
                    break;

                case EVENT_JOHNY_MOVE_8:
                    me->GetMotionMaster()->MovePoint(8, -589.597f, 379.442f, 76.082f);
                    break;
                }
            }

        }

        void Pokracuj()
        {
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            _events.ScheduleEvent(EVENT_JOHNY_SAY_2, 700);
        }

        void Set_pPlayer(Player* p_player)
        {
            pPlayer = p_player;
        }

    private:
        EventMap _events;
        Unit* myOwner;
        Player* pPlayer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new custom_johnyAI(creature);
    }
};

void AddSC_hillsbrad_foothills()
{
    new custom_skeletal_steed();
    new custom_dumass();
    new custom_orkus();
    new custom_johny();
}
