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
SDName: Npcs_Special
SD%Complete: 100
SDComment: To be used for special NPCs that are located globally.
SDCategory: NPCs
EndScriptData
*/

/* ContentData
npc_air_force_bots       80%    support for misc (invisible) guard bots in areas where player allowed to fly. Summon guards after a preset time if tagged by spell
npc_lunaclaw_spirit      80%    support for quests 6001/6002 (Body and Heart)
npc_chicken_cluck       100%    support for quest 3861 (Cluck!)
npc_dancing_flames      100%    midsummer event NPC
npc_guardian            100%    guardianAI used to prevent players from accessing off-limits areas. Not in use by SD2
npc_garments_of_quests   80%    NPC's related to all Garments of-quests 5621, 5624, 5625, 5648, 565
npc_injured_patient     100%    patients for triage-quests (6622 and 6624)
npc_doctor              100%    Gustaf Vanhowzen and Gregory Victor, quest 6622 and 6624 (Triage)
npc_kingdom_of_dalaran_quests   Misc NPC's gossip option related to quests 12791, 12794 and 12796
npc_mount_vendor        100%    Regular mount vendors all over the world. Display gossip if player doesn't meet the requirements to buy
npc_rogue_trainer        80%    Scripted trainers, so they are able to offer item 17126 for class quest 6681
npc_sayge               100%    Darkmoon event fortune teller, buff player based on answers given
npc_snake_trap_serpents  80%    AI for snakes that summoned by Snake Trap
npc_shadowfiend         100%   restore 5% of owner's mana when shadowfiend die from damage
npc_locksmith            75%    list of keys needs to be confirmed
EndContentData */

#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "World.h"

/*########
# npc_air_force_bots
#########*/

enum SpawnType
{
    SPAWNTYPE_TRIPWIRE_ROOFTOP,                             // no warning, summon Creature at smaller range
    SPAWNTYPE_ALARMBOT,                                     // cast guards mark and summon npc - if player shows up with that buff duration < 5 seconds attack
};

struct SpawnAssociation
{
    uint32 m_uiThisCreatureEntry;
    uint32 m_uiSpawnedCreatureEntry;
    SpawnType m_SpawnType;
};

enum eEnums
{
    SPELL_GUARDS_MARK               = 38067,
    AURA_DURATION_TIME_LEFT         = 5000
};

const float RANGE_TRIPWIRE          = 15.0f;
const float RANGE_GUARDS_MARK       = 50.0f;

SpawnAssociation m_aSpawnAssociations[] =
{
    {2614,  15241, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Alliance)
    {2615,  15242, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Horde)
    {21974, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Area 52)
    {21993, 15242, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Horde - Bat Rider)
    {21996, 15241, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Alliance - Gryphon)
    {21997, 21976, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Goblin - Area 52 - Zeppelin)
    {21999, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Alliance)
    {22001, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Horde)
    {22002, 15242, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Horde)
    {22003, 15241, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Ground (Alliance)
    {22063, 21976, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Goblin - Area 52)
    {22065, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Ethereal - Stormspire)
    {22066, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Scryer - Dragonhawk)
    {22068, 22064, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Ethereal - Stormspire)
    {22069, 22064, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Stormspire)
    {22070, 22067, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Scryer)
    {22071, 22067, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Scryer)
    {22078, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Aldor)
    {22079, 22077, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Aldor - Gryphon)
    {22080, 22077, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Aldor)
    {22086, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Sporeggar)
    {22087, 22085, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Sporeggar - Spore Bat)
    {22088, 22085, SPAWNTYPE_TRIPWIRE_ROOFTOP},             //Air Force Trip Wire - Rooftop (Sporeggar)
    {22090, 22089, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Toshley's Station - Flying Machine)
    {22124, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Alarm Bot (Cenarion)
    {22125, 22122, SPAWNTYPE_ALARMBOT},                     //Air Force Guard Post (Cenarion - Stormcrow)
    {22126, 22122, SPAWNTYPE_ALARMBOT}                      //Air Force Trip Wire - Rooftop (Cenarion Expedition)
};

class npc_air_force_bots : public CreatureScript
{
public:
    npc_air_force_bots() : CreatureScript("npc_air_force_bots") { }

    struct npc_air_force_botsAI : public ScriptedAI
    {
        npc_air_force_botsAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pSpawnAssoc = NULL;
            m_uiSpawnedGUID = 0;

            // find the correct spawnhandling
            static uint32 uiEntryCount = sizeof(m_aSpawnAssociations)/sizeof(SpawnAssociation);

            for (uint8 i=0; i<uiEntryCount; ++i)
            {
                if (m_aSpawnAssociations[i].m_uiThisCreatureEntry == pCreature->GetEntry())
                {
                    m_pSpawnAssoc = &m_aSpawnAssociations[i];
                    break;
                }
            }

            if (!m_pSpawnAssoc)
                sLog->outErrorDb("TCSR: Creature template entry %u has ScriptName npc_air_force_bots, but it's not handled by that script", pCreature->GetEntry());
            else
            {
                CreatureInfo const* spawnedTemplate = GetCreatureTemplateStore(m_pSpawnAssoc->m_uiSpawnedCreatureEntry);

                if (!spawnedTemplate)
                {
                    m_pSpawnAssoc = NULL;
                    sLog->outErrorDb("TCSR: Creature template entry %u does not exist in DB, which is required by npc_air_force_bots", m_pSpawnAssoc->m_uiSpawnedCreatureEntry);
                    return;
                }
            }
        }

        SpawnAssociation* m_pSpawnAssoc;
        uint64 m_uiSpawnedGUID;

        void Reset() {}

        Creature* SummonGuard()
        {
            Creature* pSummoned = me->SummonCreature(m_pSpawnAssoc->m_uiSpawnedCreatureEntry, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 300000);

            if (pSummoned)
                m_uiSpawnedGUID = pSummoned->GetGUID();
            else
            {
                sLog->outErrorDb("TCSR: npc_air_force_bots: wasn't able to spawn Creature %u", m_pSpawnAssoc->m_uiSpawnedCreatureEntry);
                m_pSpawnAssoc = NULL;
            }

            return pSummoned;
        }

        Creature* GetSummonedGuard()
        {
            Creature* pCreature = Unit::GetCreature(*me, m_uiSpawnedGUID);

            if (pCreature && pCreature->isAlive())
                return pCreature;

            return NULL;
        }

        void MoveInLineOfSight(Unit* pWho)
        {
            if (!m_pSpawnAssoc)
                return;

            if (pWho->isTargetableForAttack() && me->IsHostileTo(pWho))
            {
                Player* pPlayerTarget = pWho->GetTypeId() == TYPEID_PLAYER ? CAST_PLR(pWho) : NULL;

                // airforce guards only spawn for players
                if (!pPlayerTarget)
                    return;

                Creature* pLastSpawnedGuard = m_uiSpawnedGUID == 0 ? NULL : GetSummonedGuard();

                // prevent calling Unit::GetUnit at next MoveInLineOfSight call - speedup
                if (!pLastSpawnedGuard)
                    m_uiSpawnedGUID = 0;

                switch(m_pSpawnAssoc->m_SpawnType)
                {
                    case SPAWNTYPE_ALARMBOT:
                    {
                        if (!pWho->IsWithinDistInMap(me, RANGE_GUARDS_MARK))
                            return;

                        Aura* pMarkAura = pWho->GetAura(SPELL_GUARDS_MARK);
                        if (pMarkAura)
                        {
                            // the target wasn't able to move out of our range within 25 seconds
                            if (!pLastSpawnedGuard)
                            {
                                pLastSpawnedGuard = SummonGuard();

                                if (!pLastSpawnedGuard)
                                    return;
                            }

                            if (pMarkAura->GetDuration() < AURA_DURATION_TIME_LEFT)
                            {
                                if (!pLastSpawnedGuard->getVictim())
                                    pLastSpawnedGuard->AI()->AttackStart(pWho);
                            }
                        }
                        else
                        {
                            if (!pLastSpawnedGuard)
                                pLastSpawnedGuard = SummonGuard();

                            if (!pLastSpawnedGuard)
                                return;

                            pLastSpawnedGuard->CastSpell(pWho, SPELL_GUARDS_MARK, true);
                        }
                        break;
                    }
                    case SPAWNTYPE_TRIPWIRE_ROOFTOP:
                    {
                        if (!pWho->IsWithinDistInMap(me, RANGE_TRIPWIRE))
                            return;

                        if (!pLastSpawnedGuard)
                            pLastSpawnedGuard = SummonGuard();

                        if (!pLastSpawnedGuard)
                            return;

                        // ROOFTOP only triggers if the player is on the ground
                        if (!pPlayerTarget->IsFlying())
                        {
                            if (!pLastSpawnedGuard->getVictim())
                                pLastSpawnedGuard->AI()->AttackStart(pWho);
                        }
                        break;
                    }
                }
            }
        }
    };


    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_air_force_botsAI(creature);
    }
};

/*######
## npc_lunaclaw_spirit
######*/

enum
{
    QUEST_BODY_HEART_A      = 6001,
    QUEST_BODY_HEART_H      = 6002,

    TEXT_ID_DEFAULT         = 4714,
    TEXT_ID_PROGRESS        = 4715
};

#define GOSSIP_ITEM_GRANT   "You have thought well, spirit. I ask you to grant me the strength of your body and the strength of your heart."

class npc_lunaclaw_spirit : public CreatureScript
{
public:
    npc_lunaclaw_spirit() : CreatureScript("npc_lunaclaw_spirit") { }

    bool OnGossipHello(Player *pPlayer, Creature *pCreature)
    {
        if (pPlayer->GetQuestStatus(QUEST_BODY_HEART_A) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_BODY_HEART_H) == QUEST_STATUS_INCOMPLETE)
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GRANT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        pPlayer->SEND_GOSSIP_MENU(TEXT_ID_DEFAULT, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            pPlayer->SEND_GOSSIP_MENU(TEXT_ID_PROGRESS, pCreature->GetGUID());
            pPlayer->AreaExploredOrEventHappens(pPlayer->GetTeam() == ALLIANCE ? QUEST_BODY_HEART_A : QUEST_BODY_HEART_H);
        }
        return true;
    }
};

/*########
# npc_chicken_cluck
#########*/

#define EMOTE_HELLO         -1070004
#define EMOTE_CLUCK_TEXT    -1070006

#define QUEST_CLUCK         3861
#define FACTION_FRIENDLY    35
#define FACTION_CHICKEN     31

class npc_chicken_cluck : public CreatureScript
{
public:
    npc_chicken_cluck() : CreatureScript("npc_chicken_cluck") { }

    struct npc_chicken_cluckAI : public ScriptedAI
    {
        npc_chicken_cluckAI(Creature *c) : ScriptedAI(c) {}

        uint32 ResetFlagTimer;

        void Reset()
        {
            ResetFlagTimer = 120000;
            me->setFaction(FACTION_CHICKEN);
            me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        }

        void EnterCombat(Unit * /*who*/) {}

        void UpdateAI(const uint32 diff)
        {
            // Reset flags after a certain time has passed so that the next player has to start the 'event' again
            if (me->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER))
            {
                if (ResetFlagTimer <= diff)
                {
                    EnterEvadeMode();
                    return;
                } else ResetFlagTimer -= diff;
            }

            if (UpdateVictim())
                DoMeleeAttackIfReady();
        }

        void ReceiveEmote(Player* pPlayer, uint32 emote)
        {
            switch(emote)
            {
                case TEXTEMOTE_CHICKEN:
                    if (pPlayer->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_NONE && rand()%30 == 1)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        me->setFaction(FACTION_FRIENDLY);
                        DoScriptText(EMOTE_HELLO, me);
                    }
                    break;
                case TEXTEMOTE_CHEER:
                    if (pPlayer->GetQuestStatus(QUEST_CLUCK) == QUEST_STATUS_COMPLETE)
                    {
                        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        me->setFaction(FACTION_FRIENDLY);
                        DoScriptText(EMOTE_CLUCK_TEXT, me);
                    }
                    break;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_chicken_cluckAI(creature);
    }

    bool OnQuestAccept(Player* /*pPlayer*/, Creature* pCreature, const Quest *_Quest)
    {
        if (_Quest->GetQuestId() == QUEST_CLUCK)
            CAST_AI(npc_chicken_cluck::npc_chicken_cluckAI, pCreature->AI())->Reset();

        return true;
    }

    bool OnQuestComplete(Player* /*pPlayer*/, Creature* pCreature, const Quest *_Quest)
    {
        if (_Quest->GetQuestId() == QUEST_CLUCK)
            CAST_AI(npc_chicken_cluck::npc_chicken_cluckAI, pCreature->AI())->Reset();

        return true;
    }
};

/*######
## npc_dancing_flames
######*/

#define SPELL_BRAZIER       45423
#define SPELL_SEDUCTION     47057
#define SPELL_FIERY_AURA    45427

class npc_dancing_flames : public CreatureScript
{
public:
    npc_dancing_flames() : CreatureScript("npc_dancing_flames") { }

    struct npc_dancing_flamesAI : public ScriptedAI
    {
        npc_dancing_flamesAI(Creature *c) : ScriptedAI(c) {}

        bool active;
        uint32 can_iteract;

        void Reset()
        {
            active = true;
            can_iteract = 3500;
            DoCast(me, SPELL_BRAZIER, true);
            DoCast(me, SPELL_FIERY_AURA, false);
            float x, y, z;
            me->GetPosition(x,y,z);
            me->Relocate(x,y,z + 0.94f);
            me->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING);
            me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
            WorldPacket data;                       //send update position to client
            me->BuildHeartBeatMsg(&data);
            me->SendMessageToSet(&data,true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!active)
            {
                if (can_iteract <= diff)
                {
                    active = true;
                    can_iteract = 3500;
                    me->HandleEmoteCommand(EMOTE_ONESHOT_DANCE);
                } else can_iteract -= diff;
            }
        }

        void EnterCombat(Unit* /*who*/){}

        void ReceiveEmote(Player* pPlayer, uint32 emote)
        {
            if (me->IsWithinLOS(pPlayer->GetPositionX(),pPlayer->GetPositionY(),pPlayer->GetPositionZ()) && me->IsWithinDistInMap(pPlayer,30.0f))
            {
                me->SetInFront(pPlayer);
                active = false;

                WorldPacket data;
                me->BuildHeartBeatMsg(&data);
                me->SendMessageToSet(&data,true);
                switch(emote)
                {
                    case TEXTEMOTE_KISS:    me->HandleEmoteCommand(EMOTE_ONESHOT_SHY); break;
                    case TEXTEMOTE_WAVE:    me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE); break;
                    case TEXTEMOTE_BOW:     me->HandleEmoteCommand(EMOTE_ONESHOT_BOW); break;
                    case TEXTEMOTE_JOKE:    me->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH); break;
                    case TEXTEMOTE_DANCE:
                    {
                        if (!pPlayer->HasAura(SPELL_SEDUCTION))
                            DoCast(pPlayer, SPELL_SEDUCTION, true);
                    }
                    break;
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_dancing_flamesAI(creature);
    }
};


/*######
## Triage quest
######*/

//signed for 9623
#define SAY_DOC1    -1000201
#define SAY_DOC2    -1000202
#define SAY_DOC3    -1000203

#define DOCTOR_ALLIANCE     12939
#define DOCTOR_HORDE        12920
#define ALLIANCE_COORDS     7
#define HORDE_COORDS        6

struct Location
{
    float x, y, z, o;
};

static Location AllianceCoords[]=
{
    {-3757.38f, -4533.05f, 14.16f, 3.62f},                      // Top-far-right bunk as seen from entrance
    {-3754.36f, -4539.13f, 14.16f, 5.13f},                      // Top-far-left bunk
    {-3749.54f, -4540.25f, 14.28f, 3.34f},                      // Far-right bunk
    {-3742.10f, -4536.85f, 14.28f, 3.64f},                      // Right bunk near entrance
    {-3755.89f, -4529.07f, 14.05f, 0.57f},                      // Far-left bunk
    {-3749.51f, -4527.08f, 14.07f, 5.26f},                      // Mid-left bunk
    {-3746.37f, -4525.35f, 14.16f, 5.22f},                      // Left bunk near entrance
};

//alliance run to where
#define A_RUNTOX -3742.96f
#define A_RUNTOY -4531.52f
#define A_RUNTOZ 11.91f

static Location HordeCoords[]=
{
    {-1013.75f, -3492.59f, 62.62f, 4.34f},                      // Left, Behind
    {-1017.72f, -3490.92f, 62.62f, 4.34f},                      // Right, Behind
    {-1015.77f, -3497.15f, 62.82f, 4.34f},                      // Left, Mid
    {-1019.51f, -3495.49f, 62.82f, 4.34f},                      // Right, Mid
    {-1017.25f, -3500.85f, 62.98f, 4.34f},                      // Left, front
    {-1020.95f, -3499.21f, 62.98f, 4.34f}                       // Right, Front
};

//horde run to where
#define H_RUNTOX -1016.44f
#define H_RUNTOY -3508.48f
#define H_RUNTOZ 62.96f

const uint32 AllianceSoldierId[3] =
{
    12938,                                                  // 12938 Injured Alliance Soldier
    12936,                                                  // 12936 Badly injured Alliance Soldier
    12937                                                   // 12937 Critically injured Alliance Soldier
};

const uint32 HordeSoldierId[3] =
{
    12923,                                                  //12923 Injured Soldier
    12924,                                                  //12924 Badly injured Soldier
    12925                                                   //12925 Critically injured Soldier
};

/*######
## npc_doctor (handles both Gustaf Vanhowzen and Gregory Victor)
######*/
class npc_doctor : public CreatureScript
{
public:
    npc_doctor() : CreatureScript("npc_doctor") {}

    struct npc_doctorAI : public ScriptedAI
    {
        npc_doctorAI(Creature *c) : ScriptedAI(c) {}

        uint64 PlayerGUID;

        uint32 SummonPatient_Timer;
        uint32 SummonPatientCount;
        uint32 PatientDiedCount;
        uint32 PatientSavedCount;

        bool Event;

        std::list<uint64> Patients;
        std::vector<Location*> Coordinates;

        void Reset()
        {
            PlayerGUID = 0;

            SummonPatient_Timer = 10000;
            SummonPatientCount = 0;
            PatientDiedCount = 0;
            PatientSavedCount = 0;

            Patients.clear();
            Coordinates.clear();

            Event = false;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void BeginEvent(Player* pPlayer)
        {
            PlayerGUID = pPlayer->GetGUID();

            SummonPatient_Timer = 10000;
            SummonPatientCount = 0;
            PatientDiedCount = 0;
            PatientSavedCount = 0;

            switch(me->GetEntry())
            {
                case DOCTOR_ALLIANCE:
                    for (uint8 i = 0; i < ALLIANCE_COORDS; ++i)
                        Coordinates.push_back(&AllianceCoords[i]);
                    break;
                case DOCTOR_HORDE:
                    for (uint8 i = 0; i < HORDE_COORDS; ++i)
                        Coordinates.push_back(&HordeCoords[i]);
                    break;
            }

            Event = true;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void PatientDied(Location* Point)
        {
            Player* pPlayer = Unit::GetPlayer(*me, PlayerGUID);
            if (pPlayer && ((pPlayer->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (pPlayer->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)))
            {
                ++PatientDiedCount;

                if (PatientDiedCount > 5 && Event)
                {
                    if (pPlayer->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE)
                        pPlayer->FailQuest(6624);
                    else if (pPlayer->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)
                        pPlayer->FailQuest(6622);

                    Reset();
                    return;
                }

                Coordinates.push_back(Point);
            }
            else
                // If no player or player abandon quest in progress
                Reset();
        }

        void PatientSaved(Creature* /*soldier*/, Player* pPlayer, Location* Point)
        {
            if (pPlayer && PlayerGUID == pPlayer->GetGUID())
            {
                if ((pPlayer->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (pPlayer->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE))
                {
                    ++PatientSavedCount;

                    if (PatientSavedCount == 15)
                    {
                        if (!Patients.empty())
                        {
                            std::list<uint64>::const_iterator itr;
                            for (itr = Patients.begin(); itr != Patients.end(); ++itr)
                            {
                                if (Creature* Patient = Unit::GetCreature((*me), *itr))
                                    Patient->setDeathState(JUST_DIED);
                            }
                        }

                        if (pPlayer->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE)
                            pPlayer->AreaExploredOrEventHappens(6624);
                        else if (pPlayer->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE)
                            pPlayer->AreaExploredOrEventHappens(6622);

                        Reset();
                        return;
                    }

                    Coordinates.push_back(Point);
                }
            }
        }

        void UpdateAI(const uint32 diff);

        void EnterCombat(Unit* /*who*/){}
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const *quest)
    {
        if ((quest->GetQuestId() == 6624) || (quest->GetQuestId() == 6622))
            CAST_AI(npc_doctor::npc_doctorAI, pCreature->AI())->BeginEvent(pPlayer);

        return true;
    }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_doctorAI(creature);
    }
};

/*#####
## npc_injured_patient (handles all the patients, no matter Horde or Alliance)
#####*/

class npc_injured_patient : public CreatureScript
{
public:
    npc_injured_patient() : CreatureScript("npc_injured_patient") { }

    struct npc_injured_patientAI : public ScriptedAI
    {
        npc_injured_patientAI(Creature *c) : ScriptedAI(c) {}

        uint64 Doctorguid;
        Location* Coord;

        void Reset()
        {
            Doctorguid = 0;
            Coord = NULL;

            //no select
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

            //no regen health
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

            //to make them lay with face down
            me->SetUInt32Value(UNIT_FIELD_BYTES_1, UNIT_STAND_STATE_DEAD);

            uint32 mobId = me->GetEntry();

            switch (mobId)
            {                                                   //lower max health
                case 12923:
                case 12938:                                     //Injured Soldier
                    me->SetHealth(me->CountPctFromMaxHealth(75));
                    break;
                case 12924:
                case 12936:                                     //Badly injured Soldier
                    me->SetHealth(me->CountPctFromMaxHealth(50));
                    break;
                case 12925:
                case 12937:                                     //Critically injured Soldier
                    me->SetHealth(me->CountPctFromMaxHealth(25));
                    break;
            }
        }

        void EnterCombat(Unit* /*who*/){}

        void SpellHit(Unit *caster, const SpellEntry *spell)
        {
            if (caster->GetTypeId() == TYPEID_PLAYER && me->isAlive() && spell->Id == 20804)
            {
                if ((CAST_PLR(caster)->GetQuestStatus(6624) == QUEST_STATUS_INCOMPLETE) || (CAST_PLR(caster)->GetQuestStatus(6622) == QUEST_STATUS_INCOMPLETE))
                    if (Doctorguid)
                        if (Creature* Doctor = Unit::GetCreature(*me, Doctorguid))
                            CAST_AI(npc_doctor::npc_doctorAI, Doctor->AI())->PatientSaved(me, CAST_PLR(caster), Coord);

                //make not selectable
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                //regen health
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);

                //stand up
                me->SetUInt32Value(UNIT_FIELD_BYTES_1, UNIT_STAND_STATE_STAND);

                DoScriptText(RAND(SAY_DOC1,SAY_DOC2,SAY_DOC3), me);

                uint32 mobId = me->GetEntry();
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

                switch (mobId)
                {
                    case 12923:
                    case 12924:
                    case 12925:
                        me->GetMotionMaster()->MovePoint(0, H_RUNTOX, H_RUNTOY, H_RUNTOZ);
                        break;
                    case 12936:
                    case 12937:
                    case 12938:
                        me->GetMotionMaster()->MovePoint(0, A_RUNTOX, A_RUNTOY, A_RUNTOZ);
                        break;
                }
            }
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            //lower HP on every world tick makes it a useful counter, not officlone though
            if (me->isAlive() && me->GetHealth() > 6)
            {
                me->ModifyHealth(-5);
            }

            if (me->isAlive() && me->GetHealth() <= 6)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->setDeathState(JUST_DIED);
                me->SetFlag(UNIT_DYNAMIC_FLAGS, 32);

                if (Doctorguid)
                {
                    if (Creature* Doctor = Unit::GetCreature((*me), Doctorguid))
                        CAST_AI(npc_doctor::npc_doctorAI, Doctor->AI())->PatientDied(Coord);
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_injured_patientAI(creature);
    }
};

void npc_doctor::npc_doctorAI::UpdateAI(const uint32 diff)
{
    if (Event && SummonPatientCount >= 20)
    {
        Reset();
        return;
    }

    if (Event)
    {
        if (SummonPatient_Timer <= diff)
        {
            if (Coordinates.empty())
                return;

            std::vector<Location*>::iterator itr = Coordinates.begin()+rand()%Coordinates.size();
            uint32 patientEntry = 0;

            switch(me->GetEntry())
            {
            case DOCTOR_ALLIANCE: patientEntry = AllianceSoldierId[rand()%3]; break;
            case DOCTOR_HORDE:    patientEntry = HordeSoldierId[rand()%3]; break;
            default:
                sLog->outError("TSCR: Invalid entry for Triage doctor. Please check your database");
                return;
            }

            if (Location* Point = *itr)
            {
                if (Creature* Patient = me->SummonCreature(patientEntry, Point->x, Point->y, Point->z, Point->o, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000))
                {
                    //303, this flag appear to be required for client side item->spell to work (TARGET_SINGLE_FRIEND)
                    Patient->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

                    Patients.push_back(Patient->GetGUID());
                    CAST_AI(npc_injured_patient::npc_injured_patientAI, Patient->AI())->Doctorguid = me->GetGUID();

                    if (Point)
                        CAST_AI(npc_injured_patient::npc_injured_patientAI, Patient->AI())->Coord = Point;

                    Coordinates.erase(itr);
                }
            }
            SummonPatient_Timer = 10000;
            ++SummonPatientCount;
        } else SummonPatient_Timer -= diff;
    }
}

/*######
## npc_garments_of_quests
######*/

//TODO: get text for each NPC

enum eGarments
{
    SPELL_LESSER_HEAL_R2    = 2052,
    SPELL_FORTITUDE_R1      = 1243,

    QUEST_MOON              = 5621,
    QUEST_LIGHT_1           = 5624,
    QUEST_LIGHT_2           = 5625,
    QUEST_SPIRIT            = 5648,
    QUEST_DARKNESS          = 5650,

    ENTRY_SHAYA             = 12429,
    ENTRY_ROBERTS           = 12423,
    ENTRY_DOLF              = 12427,
    ENTRY_KORJA             = 12430,
    ENTRY_DG_KEL            = 12428,

    //used by 12429,12423,12427,12430,12428, but signed for 12429
    SAY_COMMON_HEALED       = -1000164,
    SAY_DG_KEL_THANKS       = -1000165,
    SAY_DG_KEL_GOODBYE      = -1000166,
    SAY_ROBERTS_THANKS      = -1000167,
    SAY_ROBERTS_GOODBYE     = -1000168,
    SAY_KORJA_THANKS        = -1000169,
    SAY_KORJA_GOODBYE       = -1000170,
    SAY_DOLF_THANKS         = -1000171,
    SAY_DOLF_GOODBYE        = -1000172,
    SAY_SHAYA_THANKS        = -1000173,
    SAY_SHAYA_GOODBYE       = -1000174, //signed for 21469
};

class npc_garments_of_quests : public CreatureScript
{
public:
    npc_garments_of_quests() : CreatureScript("npc_garments_of_quests") { }

    struct npc_garments_of_questsAI : public npc_escortAI
    {
        npc_garments_of_questsAI(Creature *c) : npc_escortAI(c) {Reset();}

        uint64 caster;

        bool bIsHealed;
        bool bCanRun;

        uint32 RunAwayTimer;

        void Reset()
        {
            caster = 0;

            bIsHealed = false;
            bCanRun = false;

            RunAwayTimer = 5000;

            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            //expect database to have RegenHealth=0
            me->SetHealth(me->CountPctFromMaxHealth(70));
        }

        void EnterCombat(Unit * /*who*/) {}

        void SpellHit(Unit* pCaster, const SpellEntry *Spell)
        {
            if (Spell->Id == SPELL_LESSER_HEAL_R2 || Spell->Id == SPELL_FORTITUDE_R1)
            {
                //not while in combat
                if (me->isInCombat())
                    return;

                //nothing to be done now
                if (bIsHealed && bCanRun)
                    return;

                if (pCaster->GetTypeId() == TYPEID_PLAYER)
                {
                    switch(me->GetEntry())
                    {
                        case ENTRY_SHAYA:
                            if (CAST_PLR(pCaster)->GetQuestStatus(QUEST_MOON) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (bIsHealed && !bCanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    DoScriptText(SAY_SHAYA_THANKS,me,pCaster);
                                    bCanRun = true;
                                }
                                else if (!bIsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    caster = pCaster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    DoScriptText(SAY_COMMON_HEALED,me,pCaster);
                                    bIsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_ROBERTS:
                            if (CAST_PLR(pCaster)->GetQuestStatus(QUEST_LIGHT_1) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (bIsHealed && !bCanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    DoScriptText(SAY_ROBERTS_THANKS,me,pCaster);
                                    bCanRun = true;
                                }
                                else if (!bIsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    caster = pCaster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    DoScriptText(SAY_COMMON_HEALED,me,pCaster);
                                    bIsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_DOLF:
                            if (CAST_PLR(pCaster)->GetQuestStatus(QUEST_LIGHT_2) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (bIsHealed && !bCanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    DoScriptText(SAY_DOLF_THANKS,me,pCaster);
                                    bCanRun = true;
                                }
                                else if (!bIsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    caster = pCaster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    DoScriptText(SAY_COMMON_HEALED,me,pCaster);
                                    bIsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_KORJA:
                            if (CAST_PLR(pCaster)->GetQuestStatus(QUEST_SPIRIT) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (bIsHealed && !bCanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    DoScriptText(SAY_KORJA_THANKS,me,pCaster);
                                    bCanRun = true;
                                }
                                else if (!bIsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    caster = pCaster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    DoScriptText(SAY_COMMON_HEALED,me,pCaster);
                                    bIsHealed = true;
                                }
                            }
                            break;
                        case ENTRY_DG_KEL:
                            if (CAST_PLR(pCaster)->GetQuestStatus(QUEST_DARKNESS) == QUEST_STATUS_INCOMPLETE)
                            {
                                if (bIsHealed && !bCanRun && Spell->Id == SPELL_FORTITUDE_R1)
                                {
                                    DoScriptText(SAY_DG_KEL_THANKS,me,pCaster);
                                    bCanRun = true;
                                }
                                else if (!bIsHealed && Spell->Id == SPELL_LESSER_HEAL_R2)
                                {
                                    caster = pCaster->GetGUID();
                                    me->SetStandState(UNIT_STAND_STATE_STAND);
                                    DoScriptText(SAY_COMMON_HEALED,me,pCaster);
                                    bIsHealed = true;
                                }
                            }
                            break;
                    }

                    //give quest credit, not expect any special quest objectives
                    if (bCanRun)
                        CAST_PLR(pCaster)->TalkedToCreature(me->GetEntry(),me->GetGUID());
                }
            }
        }

        void WaypointReached(uint32 /*uiPoint*/)
        {
        }

        void UpdateAI(const uint32 diff)
        {
            if (bCanRun && !me->isInCombat())
            {
                if (RunAwayTimer <= diff)
                {
                    if (Unit *pUnit = Unit::GetUnit(*me,caster))
                    {
                        switch(me->GetEntry())
                        {
                            case ENTRY_SHAYA: DoScriptText(SAY_SHAYA_GOODBYE,me,pUnit); break;
                            case ENTRY_ROBERTS: DoScriptText(SAY_ROBERTS_GOODBYE,me,pUnit); break;
                            case ENTRY_DOLF: DoScriptText(SAY_DOLF_GOODBYE,me,pUnit); break;
                            case ENTRY_KORJA: DoScriptText(SAY_KORJA_GOODBYE,me,pUnit); break;
                            case ENTRY_DG_KEL: DoScriptText(SAY_DG_KEL_GOODBYE,me,pUnit); break;
                        }

                        Start(false,true,true);
                    }
                    else
                        EnterEvadeMode();                       //something went wrong

                    RunAwayTimer = 30000;
                } else RunAwayTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_garments_of_questsAI(creature);
    }
};

/*######
## npc_guardian
######*/

#define SPELL_DEATHTOUCH                5

class npc_guardian : public CreatureScript
{
public:
    npc_guardian() : CreatureScript("npc_guardian") { }

    struct npc_guardianAI : public ScriptedAI
    {
        npc_guardianAI(Creature *c) : ScriptedAI(c) {}

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void EnterCombat(Unit * /*who*/)
        {
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            if (!UpdateVictim())
                return;

            if (me->isAttackReady())
            {
                DoCast(me->getVictim(), SPELL_DEATHTOUCH, true);
                me->resetAttackTimer();
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_guardianAI(creature);
    }
};

/*######
## npc_kingdom_of_dalaran_quests
######*/

enum eKingdomDalaran
{
    SPELL_TELEPORT_DALARAN  = 53360,
    ITEM_KT_SIGNET          = 39740,
    QUEST_MAGICAL_KINGDOM_A = 12794,
    QUEST_MAGICAL_KINGDOM_H = 12791,
    QUEST_MAGICAL_KINGDOM_N = 12796
};

#define GOSSIP_ITEM_TELEPORT_TO "I am ready to be teleported to Dalaran."

class npc_kingdom_of_dalaran_quests : public CreatureScript
{
public:
    npc_kingdom_of_dalaran_quests() : CreatureScript("npc_kingdom_of_dalaran_quests") { }
    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->HasItemCount(ITEM_KT_SIGNET,1) && (!pPlayer->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_A) ||
            !pPlayer->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_H) || !pPlayer->GetQuestRewardStatus(QUEST_MAGICAL_KINGDOM_N)))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TELEPORT_TO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->CastSpell(pPlayer,SPELL_TELEPORT_DALARAN,false);
        }
        return true;
    }
};

/*######
## npc_mount_vendor
######*/

class npc_mount_vendor : public CreatureScript
{
public:
    npc_mount_vendor() : CreatureScript("npc_mount_vendor") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        bool canBuy;
        canBuy = false;
        uint32 vendor = pCreature->GetEntry();
        uint8 race = pPlayer->getRace();

        switch (vendor)
        {
            case 384:                                           //Katie Hunter
            case 1460:                                          //Unger Statforth
            case 2357:                                          //Merideth Carlson
            case 4885:                                          //Gregor MacVince
                if (pPlayer->GetReputationRank(72) != REP_EXALTED && race != RACE_HUMAN)
                    pPlayer->SEND_GOSSIP_MENU(5855, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 1261:                                          //Veron Amberstill
                if (pPlayer->GetReputationRank(47) != REP_EXALTED && race != RACE_DWARF)
                    pPlayer->SEND_GOSSIP_MENU(5856, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 3362:                                          //Ogunaro Wolfrunner
                if (pPlayer->GetReputationRank(76) != REP_EXALTED && race != RACE_ORC)
                    pPlayer->SEND_GOSSIP_MENU(5841, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 3685:                                          //Harb Clawhoof
                if (pPlayer->GetReputationRank(81) != REP_EXALTED && race != RACE_TAUREN)
                    pPlayer->SEND_GOSSIP_MENU(5843, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 4730:                                          //Lelanai
                if (pPlayer->GetReputationRank(69) != REP_EXALTED && race != RACE_NIGHTELF)
                    pPlayer->SEND_GOSSIP_MENU(5844, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 4731:                                          //Zachariah Post
                if (pPlayer->GetReputationRank(68) != REP_EXALTED && race != RACE_UNDEAD_PLAYER)
                    pPlayer->SEND_GOSSIP_MENU(5840, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 7952:                                          //Zjolnir
                if (pPlayer->GetReputationRank(530) != REP_EXALTED && race != RACE_TROLL)
                    pPlayer->SEND_GOSSIP_MENU(5842, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 7955:                                          //Milli Featherwhistle
                if (pPlayer->GetReputationRank(54) != REP_EXALTED && race != RACE_GNOME)
                    pPlayer->SEND_GOSSIP_MENU(5857, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 16264:                                         //Winaestra
                if (pPlayer->GetReputationRank(911) != REP_EXALTED && race != RACE_BLOODELF)
                    pPlayer->SEND_GOSSIP_MENU(10305, pCreature->GetGUID());
                else canBuy = true;
                break;
            case 17584:                                         //Torallius the Pack Handler
                if (pPlayer->GetReputationRank(930) != REP_EXALTED && race != RACE_DRAENEI)
                    pPlayer->SEND_GOSSIP_MENU(10239, pCreature->GetGUID());
                else canBuy = true;
                break;
        }

        if (canBuy)
        {
            if (pCreature->isVendor())
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
            pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_TRADE)
            pPlayer->SEND_VENDORLIST(pCreature->GetGUID());

        return true;
    }
};

/*######
## npc_rogue_trainer
######*/

#define GOSSIP_HELLO_ROGUE1 "I wish to unlearn my talents"
#define GOSSIP_HELLO_ROGUE2 "<Take the letter>"
#define GOSSIP_HELLO_ROGUE3 "Purchase a Dual Talent Specialization."

class npc_rogue_trainer : public CreatureScript
{
public:
    npc_rogue_trainer() : CreatureScript("npc_rogue_trainer") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pCreature->isTrainer())
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_TEXT_TRAIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRAIN);

        if (pCreature->isCanTrainingAndResetTalentsOf(pPlayer))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE1, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_UNLEARNTALENTS);

		if (pPlayer->GetSpecsCount() == 1 && pCreature->isCanTrainingAndResetTalentsOf(pPlayer) && pPlayer->getLevel() >= sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_TRAINER, GOSSIP_HELLO_ROGUE3, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_LEARNDUALSPEC);

        if (pPlayer->getClass() == CLASS_ROGUE && pPlayer->getLevel() >= 24 && !pPlayer->HasItemCount(17126,1) && !pPlayer->GetQuestRewardStatus(6681))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_ROGUE2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->SEND_GOSSIP_MENU(5996, pCreature->GetGUID());
        } else
            pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer,21100,false);
                break;
            case GOSSIP_ACTION_TRAIN:
                pPlayer->SEND_TRAINERLIST(pCreature->GetGUID());
                break;
            case GOSSIP_OPTION_UNLEARNTALENTS:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->SendTalentWipeConfirm(pCreature->GetGUID());
                break;
            case GOSSIP_OPTION_LEARNDUALSPEC:
                if (pPlayer->GetSpecsCount() == 1 && !(pPlayer->getLevel() < sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL)))
                {
                    if (!pPlayer->HasEnoughMoney(100000))
                    {
                        pPlayer->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
                        pPlayer->PlayerTalkClass->CloseGossip();
                        break;
                    }
                    else
                    {
                        pPlayer->ModifyMoney(-100000);

                        // Cast spells that teach dual spec
                        // Both are also ImplicitTarget self and must be cast by player
                        pPlayer->CastSpell(pPlayer,63680,true,NULL,NULL,pPlayer->GetGUID());
                        pPlayer->CastSpell(pPlayer,63624,true,NULL,NULL,pPlayer->GetGUID());

                        // Should show another Gossip text with "Congratulations..."
                        pPlayer->PlayerTalkClass->CloseGossip();
                    }
                }
                break;
        }
        return true;
    }
};

/*######
## npc_sayge
######*/

#define SPELL_DMG       23768                               //dmg
#define SPELL_RES       23769                               //res
#define SPELL_ARM       23767                               //arm
#define SPELL_SPI       23738                               //spi
#define SPELL_INT       23766                               //int
#define SPELL_STM       23737                               //stm
#define SPELL_STR       23735                               //str
#define SPELL_AGI       23736                               //agi
#define SPELL_FORTUNE   23765                               //faire fortune

#define GOSSIP_HELLO_SAYGE  "Yes"
#define GOSSIP_SENDACTION_SAYGE1    "Slay the Man"
#define GOSSIP_SENDACTION_SAYGE2    "Turn him over to liege"
#define GOSSIP_SENDACTION_SAYGE3    "Confiscate the corn"
#define GOSSIP_SENDACTION_SAYGE4    "Let him go and have the corn"
#define GOSSIP_SENDACTION_SAYGE5    "Execute your friend painfully"
#define GOSSIP_SENDACTION_SAYGE6    "Execute your friend painlessly"
#define GOSSIP_SENDACTION_SAYGE7    "Let your friend go"
#define GOSSIP_SENDACTION_SAYGE8    "Confront the diplomat"
#define GOSSIP_SENDACTION_SAYGE9    "Show not so quiet defiance"
#define GOSSIP_SENDACTION_SAYGE10   "Remain quiet"
#define GOSSIP_SENDACTION_SAYGE11   "Speak against your brother openly"
#define GOSSIP_SENDACTION_SAYGE12   "Help your brother in"
#define GOSSIP_SENDACTION_SAYGE13   "Keep your brother out without letting him know"
#define GOSSIP_SENDACTION_SAYGE14   "Take credit, keep gold"
#define GOSSIP_SENDACTION_SAYGE15   "Take credit, share the gold"
#define GOSSIP_SENDACTION_SAYGE16   "Let the knight take credit"
#define GOSSIP_SENDACTION_SAYGE17   "Thanks"

class npc_sayge : public CreatureScript
{
public:
    npc_sayge() : CreatureScript("npc_sayge") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->HasSpellCooldown(SPELL_INT) ||
            pPlayer->HasSpellCooldown(SPELL_ARM) ||
            pPlayer->HasSpellCooldown(SPELL_DMG) ||
            pPlayer->HasSpellCooldown(SPELL_RES) ||
            pPlayer->HasSpellCooldown(SPELL_STR) ||
            pPlayer->HasSpellCooldown(SPELL_AGI) ||
            pPlayer->HasSpellCooldown(SPELL_STM) ||
            pPlayer->HasSpellCooldown(SPELL_SPI))
            pPlayer->SEND_GOSSIP_MENU(7393, pCreature->GetGUID());
        else
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_SAYGE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->SEND_GOSSIP_MENU(7339, pCreature->GetGUID());
        }

        return true;
    }

    void SendAction(Player* pPlayer, Creature* pCreature, uint32 uiAction)
    {
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE1,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE2,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE3,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE4,            GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                pPlayer->SEND_GOSSIP_MENU(7340, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE5,            GOSSIP_SENDER_MAIN+1, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE6,            GOSSIP_SENDER_MAIN+2, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE7,            GOSSIP_SENDER_MAIN+3, GOSSIP_ACTION_INFO_DEF);
                pPlayer->SEND_GOSSIP_MENU(7341, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE8,            GOSSIP_SENDER_MAIN+4, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE9,            GOSSIP_SENDER_MAIN+5, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE10,           GOSSIP_SENDER_MAIN+2, GOSSIP_ACTION_INFO_DEF);
                pPlayer->SEND_GOSSIP_MENU(7361, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE11,           GOSSIP_SENDER_MAIN+6, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE12,           GOSSIP_SENDER_MAIN+7, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE13,           GOSSIP_SENDER_MAIN+8, GOSSIP_ACTION_INFO_DEF);
                pPlayer->SEND_GOSSIP_MENU(7362, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE14,           GOSSIP_SENDER_MAIN+5, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE15,           GOSSIP_SENDER_MAIN+4, GOSSIP_ACTION_INFO_DEF);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE16,           GOSSIP_SENDER_MAIN+3, GOSSIP_ACTION_INFO_DEF);
                pPlayer->SEND_GOSSIP_MENU(7363, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF:
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_SENDACTION_SAYGE17,           GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                pPlayer->SEND_GOSSIP_MENU(7364, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                pCreature->CastSpell(pPlayer, SPELL_FORTUNE, false);
                pPlayer->SEND_GOSSIP_MENU(7365, pCreature->GetGUID());
                break;
        }
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiSender)
        {
            case GOSSIP_SENDER_MAIN:
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+1:
                pCreature->CastSpell(pPlayer, SPELL_DMG, false);
                pPlayer->AddSpellCooldown(SPELL_DMG,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+2:
                pCreature->CastSpell(pPlayer, SPELL_RES, false);
                pPlayer->AddSpellCooldown(SPELL_RES,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+3:
                pCreature->CastSpell(pPlayer, SPELL_ARM, false);
                pPlayer->AddSpellCooldown(SPELL_ARM,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+4:
                pCreature->CastSpell(pPlayer, SPELL_SPI, false);
                pPlayer->AddSpellCooldown(SPELL_SPI,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+5:
                pCreature->CastSpell(pPlayer, SPELL_INT, false);
                pPlayer->AddSpellCooldown(SPELL_INT,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+6:
                pCreature->CastSpell(pPlayer, SPELL_STM, false);
                pPlayer->AddSpellCooldown(SPELL_STM,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+7:
                pCreature->CastSpell(pPlayer, SPELL_STR, false);
                pPlayer->AddSpellCooldown(SPELL_STR,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
            case GOSSIP_SENDER_MAIN+8:
                pCreature->CastSpell(pPlayer, SPELL_AGI, false);
                pPlayer->AddSpellCooldown(SPELL_AGI,0,time(NULL) + 7200);
                SendAction(pPlayer, pCreature, uiAction);
                break;
        }
        return true;
    }
};

class npc_steam_tonk : public CreatureScript
{
public:
    npc_steam_tonk() : CreatureScript("npc_steam_tonk") { }

    struct npc_steam_tonkAI : public ScriptedAI
    {
        npc_steam_tonkAI(Creature *c) : ScriptedAI(c) {}

        void Reset() {}
        void EnterCombat(Unit * /*who*/) {}

        void OnPossess(bool apply)
        {
            if (apply)
            {
                // Initialize the action bar without the melee attack command
                me->InitCharmInfo();
                me->GetCharmInfo()->InitEmptyActionBar(false);

                me->SetReactState(REACT_PASSIVE);
            }
            else
                me->SetReactState(REACT_AGGRESSIVE);
        }

    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_steam_tonkAI(creature);
    }
};

#define SPELL_TONK_MINE_DETONATE 25099

class npc_tonk_mine : public CreatureScript
{
public:
    npc_tonk_mine() : CreatureScript("npc_tonk_mine") { }

    struct npc_tonk_mineAI : public ScriptedAI
    {
        npc_tonk_mineAI(Creature *c) : ScriptedAI(c)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 ExplosionTimer;

        void Reset()
        {
            ExplosionTimer = 3000;
        }

        void EnterCombat(Unit * /*who*/) {}
        void AttackStart(Unit * /*who*/) {}
        void MoveInLineOfSight(Unit * /*who*/) {}

        void UpdateAI(const uint32 diff)
        {
            if (ExplosionTimer <= diff)
            {
                DoCast(me, SPELL_TONK_MINE_DETONATE, true);
                me->setDeathState(DEAD); // unsummon it
            } else
                ExplosionTimer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_tonk_mineAI(creature);
    }
};

/*####
## npc_brewfest_reveler
####*/

class npc_brewfest_reveler : public CreatureScript
{
public:
    npc_brewfest_reveler() : CreatureScript("npc_brewfest_reveler") { }

    struct npc_brewfest_revelerAI : public ScriptedAI
    {
        npc_brewfest_revelerAI(Creature* c) : ScriptedAI(c) {}
        void ReceiveEmote(Player* pPlayer, uint32 emote)
        {
            if (!IsHolidayActive(HOLIDAY_BREWFEST))
                return;

            if (emote == TEXTEMOTE_DANCE)
                me->CastSpell(pPlayer, 41586, false);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_brewfest_revelerAI(creature);
    }
};

/*####
## npc_winter_reveler
####*/

class npc_winter_reveler : public CreatureScript
{
public:
    npc_winter_reveler() : CreatureScript("npc_winter_reveler") { }

    struct npc_winter_revelerAI : public ScriptedAI
    {
        npc_winter_revelerAI(Creature* c) : ScriptedAI(c) {}
        void ReceiveEmote(Player* pPlayer, uint32 emote)
        {
            if (!IsHolidayActive(HOLIDAY_FEAST_OF_WINTER_VEIL))
                return;
            //TODO: check auralist.
            if (pPlayer->HasAura(26218))
                return;

            if (emote == TEXTEMOTE_KISS)
            {
                me->CastSpell(me, 26218, false);
                pPlayer->CastSpell(pPlayer, 26218, false);
                switch (urand(0,2))
                {
                    case 0: me->CastSpell(pPlayer, 26207, false); break;
                    case 1: me->CastSpell(pPlayer, 26206, false); break;
                    case 2: me->CastSpell(pPlayer, 45036, false); break;
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_winter_revelerAI(creature);
    }
};

/*####
## npc_snake_trap_serpents
####*/

#define SPELL_MIND_NUMBING_POISON    25810   //Viper
#define SPELL_DEADLY_POISON          34655   //Venomous Snake
#define SPELL_CRIPPLING_POISON       30981   //Viper

#define VENOMOUS_SNAKE_TIMER 1500
#define VIPER_TIMER 3000

#define C_VIPER 19921

#define RAND 5

class npc_snake_trap : public CreatureScript
{
public:
    npc_snake_trap() : CreatureScript("npc_snake_trap_serpents") { }

    struct npc_snake_trap_serpentsAI : public ScriptedAI
    {
        npc_snake_trap_serpentsAI(Creature *c) : ScriptedAI(c) {}

        uint32 SpellTimer;
        bool IsViper;

        void EnterCombat(Unit * /*who*/) {}

        void Reset()
        {
            SpellTimer = 0;

            CreatureInfo const *Info = me->GetCreatureInfo();

            if (Info->Entry == C_VIPER)
                IsViper = true;
            else
                IsViper = false;

            me->SetMaxHealth(uint32(107 * (me->getLevel() - 40) * 0.025f));
            //Add delta to make them not all hit the same time
            uint32 delta = (rand() % 7) * 100;
            me->SetStatFloatValue(UNIT_FIELD_BASEATTACKTIME, float(Info->baseattacktime + delta));
            me->SetStatFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER , float(Info->attackpower));

            // Start attacking attacker of owner on first ai update after spawn - move in line of sight may choose better target
            if (!me->getVictim() && me->isSummon())
                if (Unit * Owner = CAST_SUM(me)->GetSummoner())
                    if (Owner->getAttackerForHelper())
                        AttackStart(Owner->getAttackerForHelper());
        }

        //Redefined for random target selection:
        void MoveInLineOfSight(Unit *who)
        {
            if (!me->getVictim() && who->isTargetableForAttack() && (me->IsHostileTo(who)) && who->isInAccessiblePlaceFor(me))
            {
                if (me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                    return;

                float attackRadius = me->GetAttackDistance(who);
                if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
                {
                    if (!(rand() % RAND))
                    {
                        me->setAttackTimer(BASE_ATTACK, (rand() % 10) * 100);
                        SpellTimer = (rand() % 10) * 100;
                        AttackStart(who);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (SpellTimer <= diff)
            {
                if (IsViper) //Viper
                {
                    if (urand(0,2) == 0) //33% chance to cast
                    {
                        uint32 spell;
                        if (urand(0,1) == 0)
                            spell = SPELL_MIND_NUMBING_POISON;
                        else
                            spell = SPELL_CRIPPLING_POISON;

                        DoCast(me->getVictim(), spell);
                    }

                    SpellTimer = VIPER_TIMER;
                }
                else //Venomous Snake
                {
                    if (urand(0,2) == 0) //33% chance to cast
                        DoCast(me->getVictim(), SPELL_DEADLY_POISON);
                    SpellTimer = VENOMOUS_SNAKE_TIMER + (rand() %5)*100;
                }
            } else SpellTimer -= diff;
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_snake_trap_serpentsAI(creature);
    }
};

#define SAY_RANDOM_MOJO0    "Now that's what I call froggy-style!"
#define SAY_RANDOM_MOJO1    "Your lily pad or mine?"
#define SAY_RANDOM_MOJO2    "This won't take long, did it?"
#define SAY_RANDOM_MOJO3    "I thought you'd never ask!"
#define SAY_RANDOM_MOJO4    "I promise not to give you warts..."
#define SAY_RANDOM_MOJO5    "Feelin' a little froggy, are ya?"
#define SAY_RANDOM_MOJO6a   "Listen, "
#define SAY_RANDOM_MOJO6b   ", I know of a little swamp not too far from here...."
#define SAY_RANDOM_MOJO7    "There's just never enough Mojo to go around..."

class mob_mojo : public CreatureScript
{
public:
    mob_mojo() : CreatureScript("mob_mojo") { }

    struct mob_mojoAI : public ScriptedAI
    {
        mob_mojoAI(Creature *c) : ScriptedAI(c) {Reset();}
        uint32 hearts;
        uint64 victimGUID;
        void Reset()
        {
            victimGUID = 0;
            hearts = 15000;
            if (Unit* own = me->GetOwner())
                me->GetMotionMaster()->MoveFollow(own,0,0);
        }
        void EnterCombat(Unit * /*who*/){}
        void UpdateAI(const uint32 diff)
        {
            if (me->HasAura(20372))
            {
                if (hearts <= diff)
                {
                    me->RemoveAurasDueToSpell(20372);
                    hearts = 15000;
                } hearts -= diff;
            }
        }
        void ReceiveEmote(Player* pPlayer, uint32 emote)
        {
            me->HandleEmoteCommand(emote);
            Unit* own = me->GetOwner();
            if (!own || own->GetTypeId() != TYPEID_PLAYER || CAST_PLR(own)->GetTeam() != pPlayer->GetTeam())
                return;
            if (emote == TEXTEMOTE_KISS)
            {
                std::string whisp = "";
                switch (rand()%8)
                {
                    case 0:whisp.append(SAY_RANDOM_MOJO0);break;
                    case 1:whisp.append(SAY_RANDOM_MOJO1);break;
                    case 2:whisp.append(SAY_RANDOM_MOJO2);break;
                    case 3:whisp.append(SAY_RANDOM_MOJO3);break;
                    case 4:whisp.append(SAY_RANDOM_MOJO4);break;
                    case 5:whisp.append(SAY_RANDOM_MOJO5);break;
                    case 6:
                        whisp.append(SAY_RANDOM_MOJO6a);
                        whisp.append(pPlayer->GetName());
                        whisp.append(SAY_RANDOM_MOJO6b);
                        break;
                    case 7:whisp.append(SAY_RANDOM_MOJO7);break;
                }
                me->MonsterWhisper(whisp.c_str(),pPlayer->GetGUID());
                if (victimGUID)
                {
                    Player* victim = Unit::GetPlayer(*me, victimGUID);
                    if (victim)
                        victim->RemoveAura(43906);//remove polymorph frog thing
                }
                me->AddAura(43906,pPlayer);//add polymorph frog thing
                victimGUID = pPlayer->GetGUID();
                DoCast(me, 20372, true);//tag.hearts
                me->GetMotionMaster()->MoveFollow(pPlayer,0,0);
                hearts = 15000;
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new mob_mojoAI(creature);
    }
};

class npc_mirror_image : public CreatureScript
{
public:
    npc_mirror_image() : CreatureScript("npc_mirror_image") { }

    struct npc_mirror_imageAI : CasterAI
    {
        npc_mirror_imageAI(Creature *c) : CasterAI(c) {}

        void InitializeAI()
        {
            CasterAI::InitializeAI();
            Unit * owner = me->GetOwner();
            if (!owner)
                return;
            // Inherit Master's Threat List (not yet implemented)
            owner->CastSpell((Unit*)NULL, 58838, true);
            // here mirror image casts on summoner spell (not present in client dbc) 49866
            // here should be auras (not present in client dbc): 35657, 35658, 35659, 35660 selfcasted by mirror images (stats related?)
            // Clone Me!
            //owner->CastSpell(me, 45204, false);
        }

        // Do not reload Creature templates on evade mode enter - prevent visual lost
        void EnterEvadeMode()
        {
            if (me->IsInEvadeMode() || !me->isAlive())
                return;

            Unit *owner = me->GetCharmerOrOwner();

            me->CombatStop(true);
            if (owner && !me->hasUnitState(UNIT_STAT_FOLLOW))
            {
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_mirror_imageAI(creature);
    }
};

class npc_ebon_gargoyle : public CreatureScript
{
public:
    npc_ebon_gargoyle() : CreatureScript("npc_ebon_gargoyle") { }

    struct npc_ebon_gargoyleAI : CasterAI
    {
        npc_ebon_gargoyleAI(Creature *c) : CasterAI(c) {}

        uint32 despawnTimer;

        void InitializeAI()
        {
            CasterAI::InitializeAI();
            Unit * owner = me->GetOwner();
            if (!owner)
                return;
            // Not needed to be despawned now
            despawnTimer = 0;
            // Find victim of Summon Gargoyle spell
            std::list<Unit*> targets;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 30);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);
            me->VisitNearbyObject(30, searcher);
            for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
                if ((*iter)->GetAura(49206,owner->GetGUID()))
                {
                    me->Attack((*iter),false);
                    break;
                }
        }

        void JustDied(Unit * /*killer*/)
        {
            // Stop Feeding Gargoyle when it dies
            if (Unit *owner = me->GetOwner())
                owner->RemoveAurasDueToSpell(50514);
        }

        // Fly away when dismissed
        void SpellHit(Unit *source, const SpellEntry *spell)
        {
            if (spell->Id != 50515 || !me->isAlive())
                return;

            Unit *owner = me->GetOwner();

            if (!owner || owner != source)
                return;

            // Stop Fighting
            me->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, true);
            // Sanctuary
            me->CastSpell(me, 54661, true);
            me->SetReactState(REACT_PASSIVE);

            // Fly Away
            me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY|MOVEMENTFLAG_ASCENDING|MOVEMENTFLAG_FLYING);
            me->SetSpeed(MOVE_FLIGHT, 0.75f, true);
            me->SetSpeed(MOVE_RUN, 0.75f, true);
            float x = me->GetPositionX() + 20 * cos(me->GetOrientation());
            float y = me->GetPositionY() + 20 * sin(me->GetOrientation());
            float z = me->GetPositionZ() + 40;
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MovePoint(0, x, y, z);

            // Despawn as soon as possible
            despawnTimer = 4 * IN_MILLISECONDS;
        }

        void UpdateAI(const uint32 diff)
        {
            if (despawnTimer > 0)
            {
                if (despawnTimer > diff)
                    despawnTimer -= diff;
                else
                {
                    me->ForcedDespawn();
                }
                return;
            }
            CasterAI::UpdateAI(diff);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_ebon_gargoyleAI(creature);
    }
};

class npc_lightwell : public CreatureScript
{
public:
    npc_lightwell() : CreatureScript("npc_lightwell") { }

    struct npc_lightwellAI : public PassiveAI
    {
        npc_lightwellAI(Creature *c) : PassiveAI(c) {}

        void Reset()
        {
            DoCast(me, 59907, false); // Spell for Lightwell Charges

            // Glyph of Lightwell (+5 charges)
            if(Unit* pOwner = me->ToTempSummon()->GetSummoner())
                if(AuraEffect* glyph = pOwner->GetAuraEffect(55673, 0))
                    if(Aura * chargesaura = me->GetAura(59907))
                        chargesaura->SetCharges(chargesaura->GetCharges() + glyph->GetAmount());

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void UpdateAI(const uint32 /*diff*/)
        {
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_lightwellAI(creature);
    }
};

enum eTrainingDummy
{
    NPC_ADVANCED_TARGET_DUMMY                  = 2674,
    NPC_TARGET_DUMMY                           = 2673,
	NPC_CATACLYSM_TARGET_DUMMY                 = 44548
};

class npc_training_dummy : public CreatureScript
{
public:
    npc_training_dummy() : CreatureScript("npc_training_dummy") { }

    struct npc_training_dummyAI : Scripted_NoMovementAI
    {
        npc_training_dummyAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            uiEntry = pCreature->GetEntry();
        }

        uint32 uiEntry;
        uint32 uiResetTimer;
        uint32 uiDespawnTimer;

        void Reset()
        {
            me->SetControlled(true,UNIT_STAT_STUNNED);//disable rotate
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);//imune to knock aways like blast wave

            uiResetTimer = 5000;
            uiDespawnTimer = 15000;
        }

        void EnterEvadeMode()
        {
            if (!_EnterEvadeMode())
                return;

            Reset();
        }

        void DamageTaken(Unit * /*done_by*/, uint32 &damage)
        {
            uiResetTimer = 5000;
            if (uiEntry != NPC_CATACLYSM_TARGET_DUMMY)
                damage = 0;
        }

        void JustDied(Unit * /*killer*/)
        {
            if (uiEntry == NPC_CATACLYSM_TARGET_DUMMY)
                me->Respawn();
        }
        void EnterCombat(Unit * /*who*/)
        {
            if (uiEntry != NPC_ADVANCED_TARGET_DUMMY && uiEntry != NPC_TARGET_DUMMY)
                return;
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!UpdateVictim())
                return;

            if (!me->hasUnitState(UNIT_STAT_STUNNED))
                me->SetControlled(true,UNIT_STAT_STUNNED);//disable rotate

            if (uiEntry != NPC_CATACLYSM_TARGET_DUMMY)
            {            
			    if (uiEntry != NPC_ADVANCED_TARGET_DUMMY && uiEntry != NPC_TARGET_DUMMY)
                {
                    if (uiResetTimer <= uiDiff)
                    {
                        EnterEvadeMode();
                        uiResetTimer = 5000;
                    }
                    else
                        uiResetTimer -= uiDiff;
                    return;
                }
                else
                {
                    if (uiDespawnTimer <= uiDiff)
                        me->ForcedDespawn();
                    else
                        uiDespawnTimer -= uiDiff;
                }
			}
        }
        void MoveInLineOfSight(Unit * /*who*/){return;}
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_training_dummyAI(pCreature);
    }
};

/*######
# npc_shadowfiend
######*/
#define GLYPH_OF_SHADOWFIEND_MANA         58227
#define GLYPH_OF_SHADOWFIEND              58228

class npc_shadowfiend : public CreatureScript
{
public:
    npc_shadowfiend() : CreatureScript("npc_shadowfiend") { }

    struct npc_shadowfiendAI : public ScriptedAI
    {
        npc_shadowfiendAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void DamageTaken(Unit* /*pKiller*/, uint32 &damage)
        {
            if (me->isSummon())
                if (Unit* pOwner = CAST_SUM(me)->GetSummoner())
                {
                    if (pOwner->HasAura(GLYPH_OF_SHADOWFIEND))
                        if (damage >= me->GetHealth())
                            pOwner->CastSpell(pOwner,GLYPH_OF_SHADOWFIEND_MANA,true);
                }
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_shadowfiendAI(creature);
    }
};

/*######
# npc_wormhole
######*/

#define GOSSIP_ENGINEERING1   "Borean Tundra."
#define GOSSIP_ENGINEERING2   "Howling Fjord."
#define GOSSIP_ENGINEERING3   "Sholazar Basin."
#define GOSSIP_ENGINEERING4   "Icecrown."
#define GOSSIP_ENGINEERING5   "Storm Peaks."

enum eWormhole
{
    SPELL_HOWLING_FJORD         = 67838,
    SPELL_SHOLAZAR_BASIN        = 67835,
    SPELL_ICECROWN              = 67836,
    SPELL_STORM_PEAKS           = 67837,

    TEXT_WORMHOLE               = 907
};

class npc_wormhole : public CreatureScript
{
public:
    npc_wormhole() : CreatureScript("npc_wormhole") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isSummon())
        {
            if (pPlayer == CAST_SUM(pCreature)->GetSummoner())
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ENGINEERING5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);

                pPlayer->PlayerTalkClass->SendGossipMenu(TEXT_WORMHOLE, pCreature->GetGUID());
            }
        }
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        bool roll = urand(0,1);

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1: //Borean Tundra
                pPlayer->CLOSE_GOSSIP_MENU();
                if (roll) //At the moment we don't have chance on spell_target_position table so we hack this
                    pPlayer->TeleportTo(571, 4305.505859f, 5450.839844f, 63.005806f, 0.627286f);
                else
                    pPlayer->TeleportTo(571, 3201.936279f, 5630.123535f, 133.658798f, 3.855272f);
                break;
            case GOSSIP_ACTION_INFO_DEF + 2: //Howling Fjord
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_HOWLING_FJORD, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 3: //Sholazar Basin
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_SHOLAZAR_BASIN, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 4: //Icecrown
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_ICECROWN, true);
                break;
            case GOSSIP_ACTION_INFO_DEF + 5: //Storm peaks
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_STORM_PEAKS, true);
                break;
        }
        return true;
    }
};

/*######
## npc_pet_trainer
######*/

enum ePetTrainer
{
    TEXT_ISHUNTER               = 5838,
    TEXT_NOTHUNTER              = 5839,
    TEXT_PETINFO                = 13474,
    TEXT_CONFIRM                = 7722
};

#define GOSSIP_PET1             "How do I train my pet?"
#define GOSSIP_PET2             "I wish to untrain my pet."
#define GOSSIP_PET_CONFIRM      "Yes, please do."

class npc_pet_trainer : public CreatureScript
{
public:
    npc_pet_trainer() : CreatureScript("npc_pet_trainer") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver())
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());

        if (pPlayer->getClass() == CLASS_HUNTER)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            if (pPlayer->GetPet() && pPlayer->GetPet()->getPetType() == HUNTER_PET)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->PlayerTalkClass->SendGossipMenu(TEXT_ISHUNTER, pCreature->GetGUID());
            return true;
        }
        pPlayer->PlayerTalkClass->SendGossipMenu(TEXT_NOTHUNTER, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pPlayer->PlayerTalkClass->SendGossipMenu(TEXT_PETINFO, pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                {
                    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_PET_CONFIRM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                    pPlayer->PlayerTalkClass->SendGossipMenu(TEXT_CONFIRM, pCreature->GetGUID());
                }
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                {
                    pPlayer->ResetPetTalents();
                    pPlayer->CLOSE_GOSSIP_MENU();
                }
                break;
        }
        return true;
    }
};

/*######
## npc_locksmith
######*/

enum eLockSmith
{
    QUEST_HOW_TO_BRAKE_IN_TO_THE_ARCATRAZ = 10704,
    QUEST_DARK_IRON_LEGACY                = 3802,
    QUEST_THE_KEY_TO_SCHOLOMANCE_A        = 5505,
    QUEST_THE_KEY_TO_SCHOLOMANCE_H        = 5511,
    QUEST_HOTTER_THAN_HELL_A              = 10758,
    QUEST_HOTTER_THAN_HELL_H              = 10764,
    QUEST_RETURN_TO_KHAGDAR               = 9837,
    QUEST_CONTAINMENT                     = 13159,

    ITEM_ARCATRAZ_KEY                     = 31084,
    ITEM_SHADOWFORGE_KEY                  = 11000,
    ITEM_SKELETON_KEY                     = 13704,
    ITEM_SHATTERED_HALLS_KEY              = 28395,
    ITEM_THE_MASTERS_KEY                  = 24490,
    ITEM_VIOLET_HOLD_KEY                  = 42482,

    SPELL_ARCATRAZ_KEY                    = 54881,
    SPELL_SHADOWFORGE_KEY                 = 54882,
    SPELL_SKELETON_KEY                    = 54883,
    SPELL_SHATTERED_HALLS_KEY             = 54884,
    SPELL_THE_MASTERS_KEY                 = 54885,
    SPELL_VIOLET_HOLD_KEY                 = 67253
};

#define GOSSIP_LOST_ARCATRAZ_KEY         "I've lost my key to the Arcatraz."
#define GOSSIP_LOST_SHADOWFORGE_KEY      "I've lost my key to the Blackrock Depths."
#define GOSSIP_LOST_SKELETON_KEY         "I've lost my key to the Scholomance."
#define GOSSIP_LOST_SHATTERED_HALLS_KEY  "I've lost my key to the Shattered Halls."
#define GOSSIP_LOST_THE_MASTERS_KEY      "I've lost my key to the Karazhan."
#define GOSSIP_LOST_VIOLET_HOLD_KEY      "I've lost my key to the Violet Hold."

class npc_locksmith : public CreatureScript
{
public:
    npc_locksmith() : CreatureScript("npc_locksmith") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        // Arcatraz Key
        if (pPlayer->GetQuestRewardStatus(QUEST_HOW_TO_BRAKE_IN_TO_THE_ARCATRAZ) && !pPlayer->HasItemCount(ITEM_ARCATRAZ_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_ARCATRAZ_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +1);

        // Shadowforge Key
        if (pPlayer->GetQuestRewardStatus(QUEST_DARK_IRON_LEGACY) && !pPlayer->HasItemCount(ITEM_SHADOWFORGE_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SHADOWFORGE_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +2);

        // Skeleton Key
        if ((pPlayer->GetQuestRewardStatus(QUEST_THE_KEY_TO_SCHOLOMANCE_A) || pPlayer->GetQuestRewardStatus(QUEST_THE_KEY_TO_SCHOLOMANCE_H)) &&
            !pPlayer->HasItemCount(ITEM_SKELETON_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SKELETON_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +3);

        // Shatered Halls Key
        if ((pPlayer->GetQuestRewardStatus(QUEST_HOTTER_THAN_HELL_A) || pPlayer->GetQuestRewardStatus(QUEST_HOTTER_THAN_HELL_H)) &&
            !pPlayer->HasItemCount(ITEM_SHATTERED_HALLS_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_SHATTERED_HALLS_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +4);

        // Master's Key
        if (pPlayer->GetQuestRewardStatus(QUEST_RETURN_TO_KHAGDAR) && !pPlayer->HasItemCount(ITEM_THE_MASTERS_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_THE_MASTERS_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +5);

        // Violet Hold Key
        if (pPlayer->GetQuestRewardStatus(QUEST_CONTAINMENT) && !pPlayer->HasItemCount(ITEM_VIOLET_HOLD_KEY, 1, true))
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_VIOLET_HOLD_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +6);

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_ARCATRAZ_KEY, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_SHADOWFORGE_KEY, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_SKELETON_KEY, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_SHATTERED_HALLS_KEY, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_THE_MASTERS_KEY, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_VIOLET_HOLD_KEY, false);
                break;
        }
        return true;
    }
};

/*######
## npc_tabard_vendor
######*/

enum
{
    QUEST_TRUE_MASTERS_OF_LIGHT = 9737,
    QUEST_THE_UNWRITTEN_PROPHECY = 9762,
    QUEST_INTO_THE_BREACH = 10259,
    QUEST_BATTLE_OF_THE_CRIMSON_WATCH = 10781,
    QUEST_SHARDS_OF_AHUNE = 11972,

    ACHIEVEMENT_EXPLORE_NORTHREND = 45,
    ACHIEVEMENT_TWENTYFIVE_TABARDS = 1021,
    ACHIEVEMENT_THE_LOREMASTER_A = 1681,
    ACHIEVEMENT_THE_LOREMASTER_H = 1682,

    ITEM_TABARD_OF_THE_HAND = 24344,
    ITEM_TABARD_OF_THE_BLOOD_KNIGHT = 25549,
    ITEM_TABARD_OF_THE_PROTECTOR = 28788,
    ITEM_OFFERING_OF_THE_SHATAR = 31408,
    ITEM_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI = 31404,
    ITEM_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI = 31405,
    ITEM_TABARD_OF_THE_SUMMER_SKIES = 35279,
    ITEM_TABARD_OF_THE_SUMMER_FLAMES = 35280,
    ITEM_TABARD_OF_THE_ACHIEVER = 40643,
    ITEM_LOREMASTERS_COLORS = 43300,
    ITEM_TABARD_OF_THE_EXPLORER = 43348,

    SPELL_TABARD_OF_THE_BLOOD_KNIGHT = 54974,
    SPELL_TABARD_OF_THE_HAND = 54976,
    SPELL_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI = 54977,
    SPELL_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI = 54982,
    SPELL_TABARD_OF_THE_ACHIEVER = 55006,
    SPELL_TABARD_OF_THE_PROTECTOR = 55008,
    SPELL_LOREMASTERS_COLORS = 58194,
    SPELL_TABARD_OF_THE_EXPLORER = 58224,
    SPELL_TABARD_OF_SUMMER_SKIES = 62768,
    SPELL_TABARD_OF_SUMMER_FLAMES = 62769
};

#define GOSSIP_LOST_TABARD_OF_BLOOD_KNIGHT "I've lost my Tabard of Blood Knight."
#define GOSSIP_LOST_TABARD_OF_THE_HAND "I've lost my Tabard of the Hand."
#define GOSSIP_LOST_TABARD_OF_THE_PROTECTOR "I've lost my Tabard of the Protector."
#define GOSSIP_LOST_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI "I've lost my Green Trophy Tabard of the Illidari."
#define GOSSIP_LOST_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI "I've lost my Purple Trophy Tabard of the Illidari."
#define GOSSIP_LOST_TABARD_OF_SUMMER_SKIES "I've lost my Tabard of Summer Skies."
#define GOSSIP_LOST_TABARD_OF_SUMMER_FLAMES "I've lost my Tabard of Summer Flames."
#define GOSSIP_LOST_LOREMASTERS_COLORS "I've lost my Loremaster's Colors."
#define GOSSIP_LOST_TABARD_OF_THE_EXPLORER "I've lost my Tabard of the Explorer."
#define GOSSIP_LOST_TABARD_OF_THE_ACHIEVER "I've lost my Tabard of the Achiever."

class npc_tabard_vendor : public CreatureScript
{
public:
    npc_tabard_vendor() : CreatureScript("npc_tabard_vendor") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        bool m_bLostBloodKnight = false;
        bool m_bLostHand = false;
        bool m_bLostProtector = false;
        bool m_bLostIllidari = false;
        bool m_bLostSummer = false;

        //Tabard of the Blood Knight
        if (pPlayer->GetQuestRewardStatus(QUEST_TRUE_MASTERS_OF_LIGHT) && !pPlayer->HasItemCount(ITEM_TABARD_OF_THE_BLOOD_KNIGHT, 1, true))
            m_bLostBloodKnight = true;

        //Tabard of the Hand
        if (pPlayer->GetQuestRewardStatus(QUEST_THE_UNWRITTEN_PROPHECY) && !pPlayer->HasItemCount(ITEM_TABARD_OF_THE_HAND, 1, true))
            m_bLostHand = true;

        //Tabard of the Protector
        if (pPlayer->GetQuestRewardStatus(QUEST_INTO_THE_BREACH) && !pPlayer->HasItemCount(ITEM_TABARD_OF_THE_PROTECTOR, 1, true))
            m_bLostProtector = true;

        //Green Trophy Tabard of the Illidari
        //Purple Trophy Tabard of the Illidari
        if (pPlayer->GetQuestRewardStatus(QUEST_BATTLE_OF_THE_CRIMSON_WATCH) &&
            (!pPlayer->HasItemCount(ITEM_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, 1, true) &&
            !pPlayer->HasItemCount(ITEM_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, 1, true) &&
            !pPlayer->HasItemCount(ITEM_OFFERING_OF_THE_SHATAR, 1, true)))
            m_bLostIllidari = true;

        //Tabard of Summer Skies
        //Tabard of Summer Flames
        if (pPlayer->GetQuestRewardStatus(QUEST_SHARDS_OF_AHUNE) &&
            !pPlayer->HasItemCount(ITEM_TABARD_OF_THE_SUMMER_SKIES, 1, true) &&
            !pPlayer->HasItemCount(ITEM_TABARD_OF_THE_SUMMER_FLAMES, 1, true))
            m_bLostSummer = true;

        if (m_bLostBloodKnight || m_bLostHand || m_bLostProtector || m_bLostIllidari || m_bLostSummer)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

            if (m_bLostBloodKnight)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_BLOOD_KNIGHT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +1);

            if (m_bLostHand)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_HAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF +2);

            if (m_bLostProtector)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_THE_PROTECTOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);

            if (m_bLostIllidari)
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
            }

            if (m_bLostSummer)
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_SUMMER_SKIES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_LOST_TABARD_OF_SUMMER_FLAMES, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+7);
            }

            pPlayer->SEND_GOSSIP_MENU(13583, pCreature->GetGUID());
        }
        else
            pPlayer->SEND_VENDORLIST(pCreature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(uiAction)
        {
            case GOSSIP_ACTION_TRADE:
                pPlayer->SEND_VENDORLIST(pCreature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+1:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_TABARD_OF_THE_BLOOD_KNIGHT, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_TABARD_OF_THE_HAND, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_TABARD_OF_THE_PROTECTOR, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_GREEN_TROPHY_TABARD_OF_THE_ILLIDARI, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_PURPLE_TROPHY_TABARD_OF_THE_ILLIDARI, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_TABARD_OF_SUMMER_SKIES, false);
                break;
            case GOSSIP_ACTION_INFO_DEF+7:
                pPlayer->CLOSE_GOSSIP_MENU();
                pPlayer->CastSpell(pPlayer, SPELL_TABARD_OF_SUMMER_FLAMES, false);
                break;
        }
        return true;
    }
};

/*######
## npc_experience
######*/

#define EXP_COST                100000//10 00 00 copper (10golds)
#define GOSSIP_TEXT_EXP         14736
#define GOSSIP_XP_OFF            "I no longer wish to gain experience."
#define GOSSIP_XP_ON           "I wish to start gaining experience again."

class npc_experience : public CreatureScript
{
public:
    npc_experience() : CreatureScript("npc_experience") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_XP_OFF, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_XP_ON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
        pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_TEXT_EXP, pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction)
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        bool noXPGain = pPlayer->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
        bool doSwitch = false;

        switch(uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1://xp off
                {
                    if (!noXPGain)//does gain xp
                        doSwitch = true;//switch to don't gain xp
                }
                break;
            case GOSSIP_ACTION_INFO_DEF + 2://xp on
                {
                    if (noXPGain)//doesn't gain xp
                        doSwitch = true;//switch to gain xp
                }
                break;
        }
        if (doSwitch)
        {
            if (!pPlayer->HasEnoughMoney(EXP_COST))
                pPlayer->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, 0, 0, 0);
            // allow only if the player is not in the battleground queue
            // TODO: remove the player from the queues instead of this check
            else if (!pPlayer->InBattlegroundQueue())
            {
                if (noXPGain)
                {
                    pPlayer->ModifyMoney(-EXP_COST);
                    pPlayer->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
                else if (!noXPGain)
                {
                    pPlayer->ModifyMoney(-EXP_COST);
                    pPlayer->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
                }
            }
        }
        pPlayer->PlayerTalkClass->CloseGossip();
        return true;
    }
};

class npc_outdoor_deathwing_flight : public CreatureScript
{
public:
    npc_outdoor_deathwing_flight() : CreatureScript("npc_outdoor_deathwing_flight") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_outdoor_deathwing_flightAI (pCreature);
    }

    struct npc_outdoor_deathwing_flightAI  : public ScriptedAI
    {
        npc_outdoor_deathwing_flightAI(Creature *c) : ScriptedAI(c) { }

        std::list<uint64> m_lPlayerGUID;
        Map* pMap;
        uint32 m_zoneId;
        uint32 m_uiZoneCheck_Timer;
        Player* pGuide;
        bool m_bIsMoving;

        void Reset()
        {
            pMap = me->GetMap();
            if(pMap)
                m_zoneId = pMap->GetZoneId(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            else
            {
                me->Kill(me);
                return;
            }

            me->SetReactState(REACT_PASSIVE);
            me->setActive(true);

            //me->ApplySpellImmune(0, IMMUNITY_ID, 94644, true); // removes triggering aura as well?
            me->CastSpell(me, 83508, true);

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            me->SetFlying(true);
            me->SetSpeed(MOVE_FLIGHT, 7.0f, true);

            HandleLooming(true);

            pGuide = NULL;
            m_bIsMoving = false;

            m_uiZoneCheck_Timer = 5000;
        }

        void JustDied(Unit* /*pKiller*/)
        {
            HandleLooming(false);
            me->SetVisibility(VISIBILITY_OFF);
            me->setActive(false);
            me->DeleteFromDB();
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spellInfo)
        {
            if(spellInfo->Id != 94644)
                return;

            if(pTarget == me->ToUnit())
            {
                me->RemoveAura(me->GetAura(94644));
                return;
            }
            if(Player* pPlayer = pTarget->ToPlayer())
            {
                pPlayer->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(5518));
                WorldPacket data(SMSG_SPELLINSTAKILLLOG, 8+8+4);
                data << uint64(me->GetGUID());
                data << uint64(pTarget->GetGUID());
                data << uint32(spellInfo->Id);
                pPlayer->SendMessageToSet(&data, true);
            }
            me->AddAura(94644, pTarget);
        }

        void EnterCombat(Unit* /*who*/){}

        void DamageTaken(Unit* pDoneBy, uint32 &damage)
        {
            if(pGuide && pDoneBy == pGuide->ToUnit() && damage > 10)
                return;

            damage = 0;

            if(pGuide || pDoneBy == me->ToUnit())
                return;

            if((pGuide = pDoneBy->ToPlayer()) != NULL)
                m_bIsMoving = FlyMove();
            else return;
        }

        bool FlyMove()
        {
            if(!pGuide)
                return false;

            Position pos;
            pGuide->GetPosition(&pos);
            float dist = me->GetDistance(pos);

            if(dist < 5.0f)
                return false;

            HandleLooming(true);
            me->GetMotionMaster()->MovePoint(2, pos);

            for(std::list<uint64>::const_iterator i = m_lPlayerGUID.begin(); i != m_lPlayerGUID.end(); ++i)
            {
                if(me->GetPlayer(*me, *i))
                    me->MonsterMoveWithSpeed(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), uint32(dist/6.8f));
            }
            return true;
        }

        void MovementInform(uint32 /*type*/, uint32 data)
        {
            if(data != 2)
                return;

            m_bIsMoving = FlyMove();
        }

        void UpdateAI(const uint32 diff)
        {
            if(m_uiZoneCheck_Timer < diff)
            {
                if(pMap)
                {
                    uint32 newZoneId = pMap->GetZoneId(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                    if(newZoneId != m_zoneId)
                    {
                        m_zoneId = newZoneId;
                        HandleLooming(false);
                        HandleLooming(true);
                    }
                }
                if(!m_bIsMoving)
                    m_bIsMoving = FlyMove();

                m_uiZoneCheck_Timer = 2000;
            }
            else m_uiZoneCheck_Timer -= diff;
        }

        void HandleLooming(bool apply)
        {
            if(apply)
            {
                if(!pMap)
                    return;

                Map::PlayerList const &lPlayers = pMap->GetPlayers();
                for(Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                {
                    if(Player* pPlayer = itr->getSource())
                    {
                        if(pPlayer->GetMap()->GetZoneId(pPlayer->GetPositionX(), pPlayer->GetPositionY(), pPlayer->GetPositionZ()) == m_zoneId)
                        {
                            if(!pPlayer->HasAura(94656))
                            {
                                m_lPlayerGUID.push_back(pPlayer->GetGUID());
                                pPlayer->CastCustomSpell(pPlayer, 94656, NULL, NULL, NULL, true, 0, 0, me->GetGUID());
                            }

                            me->setFaction(16);
                            me->SendUpdateToPlayer(pPlayer); // create/update deathwing for client
                            me->setFaction(35);
                            pPlayer->SendAurasForTarget((Unit*)me);
                        }
                    }
                }
            }
            else
            {
                std::list<uint64> lEraseGUID;
                for(std::list<uint64>::const_iterator i = m_lPlayerGUID.begin(); i != m_lPlayerGUID.end(); ++i)
                {
                    if(Player* pPlayer = me->GetPlayer(*me, *i))
                    {
                        pPlayer->RemoveAurasDueToSpell(94656);
                        me->DestroyForPlayer(pPlayer); // destroy deathwing for client
                        lEraseGUID.push_back(*i);
                    }
                }
                for(std::list<uint64>::const_iterator i = lEraseGUID.begin(); i != lEraseGUID.end(); ++i)
                    m_lPlayerGUID.remove(*i);
            }
        }
    };
};

class guardian_of_ancient_kings_holy : public CreatureScript
{
public:
    guardian_of_ancient_kings_holy() : CreatureScript("guardian_of_ancient_kings_holy") { }

    struct guardian_of_ancient_kings_holyAI: public ScriptedAI
    {
        guardian_of_ancient_kings_holyAI(Creature* c) : ScriptedAI(c) { }

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);

            if (me->isSummon())
            {
                if (Unit* pOwner = CAST_SUM(me)->GetSummoner())
                {
                    pOwner->CastSpell(pOwner, 86674, true);

                    if (Aura* aura = pOwner->GetAura(86674)) // Init Ancient Healer charges to 5
                        aura->SetCharges(5);
                }
            }
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new guardian_of_ancient_kings_holyAI(creature);
    }
};

class guardian_of_ancient_kings_prot : public CreatureScript
{
public:
    guardian_of_ancient_kings_prot() : CreatureScript("guardian_of_ancient_kings_prot") { }

    struct guardian_of_ancient_kings_protAI: public ScriptedAI
    {
        guardian_of_ancient_kings_protAI(Creature* c) : ScriptedAI(c)
        {
            init = false;
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        bool init;
        Unit* pOwner;
        uint32 update_timer;

        void Reset()
        {
            if(init)
            {
                if (me->isSummon())
                {
                    if ((pOwner = CAST_SUM(me)->GetSummoner()) != NULL)
                    {
                        me->CastSpell(pOwner, 86657, true);
                        me->SetFacingToObject(pOwner);
                    }
                }
            }
            else init = true;
            update_timer = 1000;
        }
        void MoveInLineOfSight(Unit *) {}
        void AttackStart(Unit *) {}

        void UpdateAI(const uint32 diff)
        {
            if(update_timer < diff)
            {
                if(pOwner)
                    me->SetFacingToObject(pOwner);
                update_timer = 1000;
            } else update_timer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new guardian_of_ancient_kings_protAI(creature);
    }
};

class guardian_of_ancient_kings_ret : public CreatureScript
{
public:
    guardian_of_ancient_kings_ret() : CreatureScript("guardian_of_ancient_kings_ret") { }

    struct guardian_of_ancient_kings_retAI: public ScriptedAI
    {
        guardian_of_ancient_kings_retAI(Creature* c) : ScriptedAI(c) { }

        Unit* pOwner;

        void Reset()
        {
            if (me->isSummon())
                pOwner = CAST_SUM(me)->GetSummoner();

            if(pOwner)
                pOwner->CastSpell(pOwner, 86701, true);

            me->CastSpell(me, 86703, true);
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            if(UpdateVictim())
            {
                if (me->isAttackReady())
                {
                    if (me->IsWithinMeleeRange(me->getVictim()))
                    {
                        me->AttackerStateUpdate(me->getVictim());
                        me->resetAttackTimer();

                        if(pOwner) // Add charge of Ancient Power to the Owner on dealing damage
                        {
                            if(Aura* aura = pOwner->GetAura(86700))
                            {
                                aura->RefreshDuration();
                                uint8 charges = aura->GetCharges();
                                if(charges < 20)
                                {
                                    if(charges < 1)
                                        ++charges;
                                    aura->SetCharges(++charges);
                                    aura->SetStackAmount(charges);
                                }
                            }
                            else pOwner->CastSpell(pOwner, 86700, true);
                        }
                    }
                }
            }
        }

        void JusdtDied(Unit* /*pKiller*/)
        {
            if(pOwner) // End the buff duration and unleash Ancient Power on guardian's death
                pOwner->RemoveAurasDueToSpell(86698);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new guardian_of_ancient_kings_retAI(creature);
    }
};

// npc_title_restorer
struct RestoreTitles
{
    uint32 title;
    uint32 achv_a;
    uint32 achv_b;
    uint32 quest;
};
const RestoreTitles restore_titles[]=
{
    {122,1402,0,0},     // Conqueror of Naxxramas
    {143,2188,0,0},     // Jenkins
    {84,1563,0,0},      // Chef
    {125,7520,0,0},     // Loremaster
    {172,4477,0,0},     // the Patient

    {83,1516,0,0},      // Salty
    {78,46,0,0},        // the Explorer
    {81,978,0,0},       // the Seeker
    {144,871,0,0},      // Bloodsail Admiral
    {77,5374,0,0},      // the Exalted

    {132,953,0,0},      // Guardian of Cenarius
    {176,4598,0,0},     // of the Ashen Verdict
    {131,945,0,0},      // The Argent Champion
    {64,431,0,0},       // Hand of A'dal
    {145,2336,0,0},     // The Insane

    {130,948,762,0},    // Ambassador
    {79,942,943,0},     // the Diplomat
    {53,0,0,10888},     // Champion of the Naaru
    {46,0,0,8743},      // Scarab Lord
    {63,0,0,11549}      // of the Shattered Sun
};
class npc_title_restorer : public CreatureScript
{
public:
    npc_title_restorer() : CreatureScript("npc_title_restorer") { }

    bool OnGossipHello(Player* plr, Creature* c)
    {
        plr->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I would like to restore my lost titles", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        plr->SEND_GOSSIP_MENU(1,c->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* plr, Creature* c, uint32 sender, uint32 action)
    {
        if (!plr)
            return false;

        uint32 count = 0;
        if (action == GOSSIP_ACTION_INFO_DEF+1)
        {
            for (int i = 0; i < 20; i++)
            {
                if (DoRestoreTitle(plr, restore_titles[i]))
                    count++;
            }
        }
        plr->CLOSE_GOSSIP_MENU();

        char output[128];
        sprintf(output, "total of %d lost titles have been restored", count);
        if (c)
            c->MonsterSay(output, LANG_UNIVERSAL, plr->GetGUID());

        return true;
    }

    bool DoRestoreTitle(Player* plr, RestoreTitles restore)
    {
        const CharTitlesEntry *titleInfo = sCharTitlesStore.LookupEntry(restore.title);
        if (!titleInfo || plr->HasTitle(titleInfo))
            return false;

        const AchievementEntry* achiev = NULL;
        if (restore.achv_a)
            if ((achiev = sAchievementStore.LookupEntry(restore.achv_a)))
                if (plr->GetAchievementMgr().HasAchieved(achiev))
                {
                    plr->SetTitle(titleInfo);
                    return true;
                }
        if (restore.achv_b)
            if ((achiev = sAchievementStore.LookupEntry(restore.achv_b)))
                if (plr->GetAchievementMgr().HasAchieved(achiev))
                {
                    plr->SetTitle(titleInfo);
                    return true;
                }
        if (restore.quest && (plr->GetQuestStatus(restore.quest) == QUEST_STATUS_COMPLETE))
        {
            plr->SetTitle(titleInfo);
            return true;
        }
        return false;
    }
};

class npc_tail_receipe_giver : public CreatureScript
{
public:
    npc_tail_receipe_giver() : CreatureScript("npc_tail_receipe_giver") { }
    
    bool OnQuestReward(Player* pPlayer, Creature* /*pCreature*/, const Quest* pQuest, uint32 /*rew*/)
    {
        if(!pQuest || !pPlayer)
            return true;

        switch(pQuest->GetQuestId())
        {
        case 314222:
            pPlayer->learnSpell(75298, false);
            break;
        case 314223:
            pPlayer->learnSpell(75288, false);
            break;
        case 314224:
            pPlayer->learnSpell(75300, false);
            break;
        case 314225:
            pPlayer->learnSpell(75299, false);
            break;
        case 314226:
            pPlayer->learnSpell(75306, false);
            break;
        case 314227:
            pPlayer->learnSpell(75307, false);
            break;
        case 314228:
            pPlayer->learnSpell(75305, false);
            break;
        case 314229:
            pPlayer->learnSpell(75304, false);
            break;
        case 314230:
            pPlayer->learnSpell(75302, false);
            break;
        case 314231:
            pPlayer->learnSpell(75303, false);
            break;
        case 314232:
            pPlayer->learnSpell(75301, false);
            break;
        case 314233:
            pPlayer->learnSpell(75289, false);
            break;
        case 314234:
            pPlayer->learnSpell(75308, false);
            break;
        case 314235:
            pPlayer->learnSpell(75309, false);
            break;
        case 314236:
            pPlayer->learnSpell(75310, false);
            break;
        default: break;
        }
        return false;
    }
};

class npc_thrall_maelstrom : public CreatureScript
{
public:
    npc_thrall_maelstrom() : CreatureScript("npc_thrall_maelstrom") { }

    struct npc_thrall_maelstromAI: public ScriptedAI
    {
        npc_thrall_maelstromAI(Creature* c): ScriptedAI(c) { }

        std::list<uint64> m_lPlayerGUID;

        void Reset()
        {
            m_lPlayerGUID.clear();
            me->CastSpell(me, 28892, true);
        }

        void MoveInLineOfSight(Unit* who)
        {
            Player* pTarget = who->ToPlayer();
            if(!pTarget)
                return;

            uint64 guid = who->GetGUID();
            for(std::list<uint64>::const_iterator i = m_lPlayerGUID.begin(); i != m_lPlayerGUID.end(); ++i)
            {
                if(Player* pPlayer = me->GetPlayer(*me, *i))
                {
                    if(pPlayer->GetGUID() == guid)
                        return;
                }
            }

            m_lPlayerGUID.push_back(guid);

            WorldPacket data(SMSG_WEATHER, (4+4+4));
            data << uint32(WEATHER_STATE_HEAVY_RAIN) << float(0.9999f) << uint8(0);
            pTarget->SendMessageToSet(&data, true);

            pTarget->SendCinematicStart(173);
        }
    };
    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_thrall_maelstromAI(creature);
    }
};

class quest_trigger : public CreatureScript
{
public:
    quest_trigger() : CreatureScript("quest_trigger") { }

    struct quest_triggerAI: public Scripted_NoMovementAI
    {
        quest_triggerAI(Creature* c): Scripted_NoMovementAI(c)
        {
            killcredit = 0;
            subname = "Undefined";

            QueryResult qr = WorldDatabase.PQuery("SELECT questcredit FROM ice_quest_credit WHERE guid=%u LIMIT 1;",me->GetDBTableGUIDLow());
            if(qr)
            {
                if(Field* fd = qr->Fetch())
                    killcredit = fd[0].GetUInt32();
            }
            /*QueryResult qr2 = WorldDatabase.PQuery("SELECT title FROM quest_template WHERE ReqCreatureOrGOId1=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId2=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId3=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId4=%u;",killcredit,killcredit,killcredit,killcredit);
            if(qr2)
            {
                Field* fd = qr2->Fetch();
                subname = fd[0].GetCppString();
            }

            me->SetName(subname);*/
        }

        void DoAction(const int32 action)
        {
            if(action == 1)
            {
                QueryResult qr = WorldDatabase.PQuery("SELECT questcredit FROM ice_quest_credit WHERE guid=%u LIMIT 1;",me->GetDBTableGUIDLow());
                if(qr)
                {
                    if(Field* fd = qr->Fetch())
                        killcredit = fd[0].GetUInt32();
                }
                /*QueryResult qr2 = WorldDatabase.PQuery("SELECT title FROM quest_template WHERE ReqCreatureOrGOId1=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId2=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId3=%u UNION \
                                                    SELECT title FROM quest_template WHERE ReqCreatureOrGOId4=%u;",killcredit,killcredit,killcredit,killcredit);
                if(qr2)
                {
                    Field* fd = qr2->Fetch();
                    subname = fd[0].GetCppString();
                }

                me->SetName(subname);*/
            }
        }

        uint32 killcredit;
        std::string subname;
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new quest_triggerAI(creature);
    }
};

class npc_ring_of_frost : public CreatureScript
{
public:
    npc_ring_of_frost() : CreatureScript("npc_ring_of_frost") { }

    struct npc_ring_of_frostAI : public ScriptedAI
    {
        npc_ring_of_frostAI(Creature *c) : ScriptedAI(c) {}
        bool Isready;
        uint32 timer;

        void Reset()
        {
            timer = 3000; // 3sec
            Isready = false;
        }

        void InitializeAI()
        {
        ScriptedAI::InitializeAI();
        Unit * owner = me->GetOwner();
        if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
            return;

        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

            // Remove other ring spawned by the player
            {
            CellPair pair(Trinity::ComputeCellPair(owner->GetPositionX(), owner->GetPositionY()));
            Cell cell(pair);
            cell.data.Part.reserved = ALL_DISTRICT;
            cell.SetNoCreate();

            std::list<Creature*> templist;
            Trinity::AllCreaturesOfEntryInGrid check(owner, me->GetEntry());
            Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInGrid> searcher(owner, templist, check);

            TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInGrid>, GridTypeMapContainer> visitor(searcher);
            cell.Visit(pair, visitor, *(owner->GetMap()));

            if (!templist.empty())
                for (std::list<Creature*>::const_iterator itr = templist.begin(); itr != templist.end(); ++itr)
                    if ((*itr)->GetOwner() == me->GetOwner() && *itr != me)
                        (*itr)->DisappearAndDie();
                        templist.clear();
            }
        }

        void EnterEvadeMode() { return; }

        void CheckIfMoveInRing(Unit *who)
        {
            if (who->isAlive() && me->IsInRange(who, 2.0f, 4.7f) && !who->HasAura(82691)/*<= target already frozen*/ && !who->HasAura(91264)/*<= target is immune*/ && me->IsWithinLOSInMap(who) && Isready)
                me->CastSpell(who, 82691, true);
        }

        void UpdateAI(const uint32 diff)
        {
            if (timer <= diff)
            {
                if (!Isready)
                {
                    Isready = true;
                    timer = 9000; // 9sec
                }
                else
                    me->DisappearAndDie();
            }
            else
                timer -= diff;

            // Find all the enemies
            std::list<Unit*> targets;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 5.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);
            me->VisitNearbyObject(5.0f, searcher);
            for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
                CheckIfMoveInRing(*iter);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ring_of_frostAI(pCreature);
    }
};

class npc_reforger: public CreatureScript
{
public:
    npc_reforger(): CreatureScript("npc_reforger") { };

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I would like to reforge an item", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        pPlayer->SEND_GOSSIP_MENU(4714,pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
    {
        if(action == GOSSIP_ACTION_INFO_DEF+1)
        {
            WorldPacket data(SMSG_REFORGING_OPEN,8,true);
            data << uint64(pCreature->GetGUID());
            pPlayer->SendMessageToSet(&data,true);
            pPlayer->CLOSE_GOSSIP_MENU();
            return true;
        }
        pPlayer->CLOSE_GOSSIP_MENU();
        return false;
    }
};

class npc_unmuter: public CreatureScript
{
public:
    npc_unmuter(): CreatureScript("npc_unmuter") { };

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Jsem postava z WotLK a nemuzu mluvit (step 1)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Jsem postava z WotLK a nemuzu mluvit (step 2)", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
        pPlayer->SEND_GOSSIP_MENU(1,pCreature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
    {
        if(action == GOSSIP_ACTION_INFO_DEF+1)
        {
            if (pPlayer->GetTeamId() == TEAM_ALLIANCE)
            {
                pPlayer->removeSpell(668);
                //pPlayer->GetSession()->KickPlayer();
            }
            else
            {
                pPlayer->removeSpell(669);
                //pPlayer->GetSession()->KickPlayer();
            }
            //return true;
        }
        if(action == GOSSIP_ACTION_INFO_DEF+2)
        {
            if (pPlayer->GetTeamId() == TEAM_ALLIANCE)
            {
                pPlayer->learnSpell(668, false);
                //pPlayer->GetSession()->KickPlayer();
            }
            else
            {
                pPlayer->learnSpell(669, false);
                //pPlayer->GetSession()->KickPlayer();
            }
            //return true;
        }
        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

/* Power Word Barrier */

class npc_power_word_barrier : public CreatureScript
{
public:
    npc_power_word_barrier() : CreatureScript("npc_power_word_barrier") { }

    struct npc_power_word_barrierAI : public ScriptedAI
    {
        npc_power_word_barrierAI(Creature *pCreature) : ScriptedAI(pCreature) {}
        
        bool checker;
        uint32 cron; // Duration
        
        void Reset()
        {
            checker = false;
            cron = 10000;
            DoCast(me, 81781);
        }

        void InitializeAI()
        {
            ScriptedAI::InitializeAI();
            Unit * owner = me->GetOwner();
            if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                return;

            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void BarrierChecker(Unit *who)
        {
            if (who->isAlive() && !who->HasAura(81782))
                me->CastSpell(who, 81782, true);
            if (who->isAlive() && who->HasAura(81782))
            {

                if (AuraEffect const* aur = who->GetAuraEffect(81782,0))
                    aur->GetBase()->SetDuration(GetSpellMaxDuration(aur->GetSpellProto()), true);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (cron <= diff)
            {
                if (!checker)
                {
                    checker = true;
                    cron = 10000;   //10 seconds
                }
                else
                    me->DisappearAndDie();
            }
            else
                cron -= diff;

            //Check friendly entities
            std::list<Unit*> targets;
            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(me, me, 7.0f);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(me, targets, u_check);

            me->VisitNearbyObject(7.0f, searcher);
            for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
                BarrierChecker(*iter);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_power_word_barrierAI(pCreature);
    }
};

/*######
## npc_flame_orb
######*/

enum eFlameOrb
{
    SPELL_FLAME_ORB_DAMAGE_VISUAL   = 86719,
    SPELL_FLAME_ORB_DAMAGE          = 82739,
    SPELL_FLAME_ORB_FIRE_POWER_EXPLODE = 83619,
    FLAME_ORB_DISTANCE              = 120
};

class npc_flame_orb : public CreatureScript
{
public:
    npc_flame_orb() : CreatureScript("npc_flame_orb") {}

    struct npc_flame_orbAI : public ScriptedAI
    {
        npc_flame_orbAI(Creature *c) : ScriptedAI(c) 
        {
            x = me->GetPositionX();
            y = me->GetPositionY();
            if(Unit* owner = me->GetOwner())
            {
                z = owner->GetPositionZ()+2;
                angle = owner->GetAngle(me);
            }
            o = me->GetOrientation();
            me->NearTeleportTo(x, y, z, o, true);
            newx = me->GetPositionX() + FLAME_ORB_DISTANCE/2 * cos(angle);
            newy = me->GetPositionY() + FLAME_ORB_DISTANCE/2 * sin(angle);
            CombatCheck = false;
        }
        
        float x,y,z,o,newx,newy,angle;
        bool CombatCheck;
        uint32 uiDespawnTimer;
        uint32 uiDespawnCheckTimer;
        uint32 uiDamageTimer;

        void EnterCombat(Unit* /*target*/)
        {
            me->GetMotionMaster()->MoveCharge(newx, newy, z, 1.14286f); // Normal speed
            uiDespawnTimer = 15*IN_MILLISECONDS;
            CombatCheck = true;
        }
        
        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
            me->AddUnitMovementFlag(MOVEMENTFLAG_FLYING);
            me->SetReactState(REACT_PASSIVE);
            if (CombatCheck == true)
                uiDespawnTimer = 15*IN_MILLISECONDS;
            else
                uiDespawnTimer = 4*IN_MILLISECONDS;
            uiDamageTimer = 1*IN_MILLISECONDS;
            me->GetMotionMaster()->MovePoint(0, newx, newy, z);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!me->isInCombat() && CombatCheck == false)
            {
                me->SetSpeed(MOVE_RUN, 2, true);
                me->SetSpeed(MOVE_FLIGHT, 2, true);
            }

            if (uiDespawnTimer <= diff)
            {
                if (Unit* owner = me->GetOwner())
                    if (AuraEffect* aureff = owner->GetAuraEffect(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE,SPELLFAMILY_MAGE,31,EFFECT_0))
                        if (const SpellEntry* spellinfo = aureff->GetSpellProto())
                            if (roll_chance_i(spellinfo->GetProcChance()))
                                owner->CastSpell(me, SPELL_FLAME_ORB_FIRE_POWER_EXPLODE, true);
                me->SetVisibility(VISIBILITY_OFF);
                me->DisappearAndDie();
            }
            else
                uiDespawnTimer -= diff;

            if (uiDamageTimer <= diff)
            {
                if (Unit* target = me->SelectNearestTarget(20))
                {
                    me->CastSpell(target, SPELL_FLAME_ORB_DAMAGE_VISUAL, false);
                    if(Unit* owner = me->GetOwner())
                        owner->CastSpell(target, SPELL_FLAME_ORB_DAMAGE, true);
                }

                uiDamageTimer = 1*IN_MILLISECONDS;
            }
            else
                uiDamageTimer -= diff;
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_flame_orbAI(creature);
    }
};

class npc_jailer: public CreatureScript
{
   public:
       npc_jailer(): CreatureScript("npc_jailer") {};

       struct npc_jailerAI: public ScriptedAI
       {
           npc_jailerAI(Creature* c): ScriptedAI(c)
           {
               Reset();
           }

           uint32 casovac;

           void Reset()
           {
               casovac = 30000;
           }

           void UpdateAI(const uint32 diff)
           {
               if (casovac <= diff)
               {
                   switch(urand(0,6))
                   {
                       case 0:
                           me->MonsterYell("Do you like bananas?",LANG_UNIVERSAL,0);
                           break;
                       case 1:
                           me->MonsterYell("Come to me! I need bananas!",LANG_UNIVERSAL,0);
                           break;
                       case 2:
                           me->MonsterYell("All work and no play makes Jack a dull boy.",LANG_UNIVERSAL,0);
                           break;
                       case 3:
                           me->MonsterYell("Venku zadna pravda neni!",LANG_UNIVERSAL,0);
                           break;
                       case 4:
                           me->MonsterYell("Zivot bez bolesti nema smysl.",LANG_UNIVERSAL,0);
                           break;
                       case 5:
                           me->MonsterSay("Switch: Tos podelal!",LANG_UNIVERSAL,0);
                           break;
                       case 6:
                           me->MonsterYell("Vsichni jsou mrtvi, Dave",LANG_UNIVERSAL,0);
                           break;
                   }
                   casovac = 30000;
               } else casovac -= diff;
           }
       };

       CreatureAI* GetAI(Creature* c) const
       {
           return new npc_jailerAI(c);
       }
};

class boss_event_jarmila: public CreatureScript
{
    public:
        boss_event_jarmila(): CreatureScript("boss_event_jarmila") {}

        struct boss_event_jarmilaAI: public ScriptedAI
        {
            boss_event_jarmilaAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 QuakeTimer;
            uint32 FearTimer;
            uint8 JumpPhase;
            uint32 JumpTimer;
            Unit* JumpTarget;
            Creature* MyPet;
            uint32 FeedPetTimer;
            uint32 RandomShotTimer;
            uint32 SmashTimer;
            uint32 ThunderclapTimer;

            void Reset()
            {
                QuakeTimer = 5000;
                FearTimer = 12000;
                JumpPhase = 0;
                JumpTimer = 0;
                me->setPowerType(POWER_MANA);
                MyPet = NULL;
                FeedPetTimer = 5000;
                RandomShotTimer = 2000;
                SmashTimer = 6000;
                ThunderclapTimer = 1000;
            }

            void EnterCombat(Unit* pWho)
            {
                me->MonsterYell("I served the master.. but it wasn't enough!",LANG_UNIVERSAL,0);
                me->CastSpell(me, 37964, true);
                me->CastSpell(me, 72148, true);
            }

            void SummonedCreatureDespawn(Creature* pCreature)
            {
                if (pCreature->GetEntry() == 99902)
                    MyPet = NULL;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (JumpPhase < 1 || JumpPhase > 35)
                {
                    if (QuakeTimer <= diff)
                    {
                        if (ThunderclapTimer < 3000)
                            ThunderclapTimer += 3000;

                        int32 bp0 = 35000;
                        me->CastCustomSpell(me->getVictim(), 55101, &bp0, 0, 0, false);
                        QuakeTimer = 25000;
                    } else QuakeTimer -= diff;

                    if (ThunderclapTimer <= diff)
                    {
                        ThunderclapTimer = urand(5000,15000);
                    } else ThunderclapTimer -= diff;

                    if (JumpPhase > 35)
                    {
                        if (SmashTimer <= diff)
                        {
                            if (QuakeTimer < 4000)
                                QuakeTimer += 4000;
                            if (ThunderclapTimer < 4000)
                                ThunderclapTimer += 4000;

                            me->CastSpell(me, 62339, false);

                            SmashTimer = urand(10000,15000);
                        } else SmashTimer -= diff;
                    }
                }

                if (me->GetHealthPct() > 50.0f && me->GetHealthPct() < 75.0f)
                {
                    if (FearTimer <= diff)
                    {
                        if (Unit* plTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            me->CastSpell(plTarget, 12542, false);
                        FearTimer = urand(14000,18000);
                    } else FearTimer -= diff;
                }

                if (me->GetHealthPct() < 50.0f && JumpPhase == 0)
                {
                    me->MonsterYell("I can jump high and high. And you?",LANG_UNIVERSAL,0);
                    JumpPhase = 1;
                    if ((JumpTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true)) != NULL)
                    {
                        me->getThreatManager().resetAllAggro();
                        me->AddThreat(JumpTarget, 100000.0f);
                        me->GetMotionMaster()->MoveJump(JumpTarget->GetPositionX(),JumpTarget->GetPositionY(),JumpTarget->GetPositionZ(),20.0f,10.0f);
                        JumpTimer = 2000;
                    }
                    me->LoadEquipment(99901);
                    me->setPowerType(POWER_RUNIC_POWER);
                    me->SetMaxPower(POWER_RUNIC_POWER, 1000);
                    me->SetPower(POWER_RUNIC_POWER, 1000);
                }
                if (JumpPhase > 0 && JumpPhase <= 20)
                {
                    if (JumpTimer <= diff)
                    {
                        if (JumpTarget)
                        {
                            if ((JumpPhase % 2) > 0)
                            {
                                me->CastSpell(JumpTarget, 36093, true);
                                me->ModifyPower(POWER_RUNIC_POWER, -100);
                                JumpPhase++;
                                JumpTimer = 500;
                            }
                            else
                            {
                                JumpPhase++;
                                if ((JumpTarget = SelectTarget(SELECT_TARGET_FARTHEST, 0, 100.0f, true)) != NULL)
                                {
                                    me->getThreatManager().resetAllAggro();
                                    me->AddThreat(JumpTarget, 100000.0f);
                                    me->GetMotionMaster()->MoveJump(JumpTarget->GetPositionX(),JumpTarget->GetPositionY(),JumpTarget->GetPositionZ(),20.0f,10.0f);
                                    JumpTimer = 2000;
                                }
                            }
                        }
                    } else JumpTimer -= diff;
                }
                if (JumpPhase > 20 && JumpPhase < 25) //moje degenerace, jen pro sichr
                {
                    JumpPhase = 30;
                    me->LoadEquipment(0);
                    me->setPowerType(POWER_FOCUS);
                    me->SetMaxPower(POWER_FOCUS,200);
                    me->SetPower(POWER_FOCUS,200);
                    me->GetMotionMaster()->MovementExpired();
                    me->GetMotionMaster()->Clear();
                    me->StopMoving();
                    DoStartNoMovement(me->getVictim());
                    me->GetMotionMaster()->MoveIdle();
                    MyPet = me->SummonCreature(99902,0,0,0,0,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,10000);
                }
                if (JumpPhase == 30)
                {
                    if (FeedPetTimer <= diff)
                    {
                        me->MonsterYell("Let the storm be with you, Stormbite!",0,0);
                        if (MyPet)
                        {
                            me->CastSpell(MyPet, 52202, true);
                            MyPet->SetPower(POWER_FOCUS,100);
                            me->DealHeal(MyPet,MyPet->GetMaxHealth()*0.1f);
                        }
                        FeedPetTimer = urand(15000,20000);
                    } else FeedPetTimer -= diff;

                    if (RandomShotTimer <= diff)
                    {
                        RandomShotTimer = 1000;
                        Unit* plTarget = SelectTarget(SELECT_TARGET_FARTHEST, 0, 100.0f, true);
                        if (plTarget)
                        {
                            me->Attack(plTarget, false);
                            int32 bp0 = 10000;
                            switch (urand(0,2))
                            {
                                case 0:
                                default:
                                    // Aimed Shot
                                    bp0 = 25500;
                                    me->CastCustomSpell(plTarget, 59243, &bp0, 0, 0, false);
                                    RandomShotTimer = 4500;
                                    break;
                                case 1:
                                    // Arcane Shot
                                    bp0 = 10000;
                                    me->CastCustomSpell(plTarget, 69989, &bp0, 0, 0, false);
                                    RandomShotTimer = 1000;
                                    break;
                                case 2:
                                    // Explosive Shot
                                    me->CastSpell(plTarget, 71126, true);
                                    RandomShotTimer = 3500;
                                    break;
                            }
                        }
                    } else RandomShotTimer -= diff;
                }
                if (me->GetHealthPct() < 25.0f && JumpPhase < 40)
                {
                    JumpPhase = 40;
                    me->CastSpell(me, 71599, true);
                    me->SetDisplayId(29792);
                    me->CastSpell(me, 69491, true);
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_event_jarmilaAI(c);
        }
};

class boss_event_jarmila_pet: public CreatureScript
{
    public:
        boss_event_jarmila_pet(): CreatureScript("boss_event_jarmila_pet") {}

        struct boss_event_jarmila_petAI: ScriptedAI
        {
            boss_event_jarmila_petAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 FatalBiteTimer;

            void Reset()
            {
                me->setPowerType(POWER_FOCUS);
                me->SetMaxPower(POWER_FOCUS,100);
                me->SetPower(POWER_FOCUS,100);

                FatalBiteTimer = 2000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (FatalBiteTimer <= diff)
                {
                    int32 bp0 = 25000;
                    me->CastCustomSpell(me->getVictim(), 54663, &bp0, 0, 0, false);
                    me->ModifyPower(POWER_FOCUS,-35);
                    FatalBiteTimer = urand(6000,12000);
                } else FatalBiteTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new boss_event_jarmila_petAI(c);
        }
};

class npc_odevzdavac: public CreatureScript
{
    public:
        npc_odevzdavac(): CreatureScript("npc_odevzdavac") {}

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I want to give you bananas.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->SEND_GOSSIP_MENU(1,pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 uiAction)
        {
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                 QueryResult pocet = ScriptDatabase.PQuery("SELECT count FROM ice_bananky WHERE guid = %u AND done = 0", pPlayer->GetGUID()); // select punishment count

                 uint32 counter = 0;
                 if (pocet != 0)
                 {
                     Field* field = pocet->Fetch();
                     counter = field[0].GetUInt32();
                 }

                 if (pPlayer->GetItemCount(54619) >= counter)
                 {
                       pPlayer->GetSession()->SendNotification("Ok, your punishment is finished.");
                       pPlayer->RemoveAurasDueToSpell(15007); // Remove Ressurect Sickness
                       ScriptDatabase.PExecute("UPDATE ice_bananky SET done = 1 WHERE guid = %u", pPlayer->GetGUID()); // SET done = 1

                       if (pPlayer->GetTeamId() == TEAM_ALLIANCE) // If Alliance
                           pPlayer->TeleportTo(0,-8833.38f,628.628f,95.0f,0.0f); // Teleport to Stormwind
                       else // If Horde
                           pPlayer->TeleportTo(1,1571.49f,-4398.65f,16.0f,0.0f); // Teleport to Orgrimmar

                       pPlayer->DestroyItemCount(54619, pPlayer->GetItemCount(54619), true); // Remove all bananas

                 }
                 else // If not item count
                 {
                     pPlayer->GetSession()->SendNotification("You must own %u bananas.",counter);
                 }
            }
            return true;
        }
};
class npc_areatrigger_completer: public CreatureScript
{
    public:
        npc_areatrigger_completer(): CreatureScript("npc_areatrigger_completer") {}

        struct npc_areatrigger_completerAI: ScriptedAI
        {
            npc_areatrigger_completerAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 quest; // Quest def
            float dist; // Distance def

            void Reset()
            {
                me->SetVisibility(VISIBILITY_OFF); // Set Creature invisible for Players

                QueryResult qResult = WorldDatabase.PQuery("SELECT quest,distance FROM creature_areatrigger_completer WHERE guid = %u", me->GetDBTableGUIDLow());

                if (qResult != 0)
                {
                    Field* fields = qResult->Fetch();
                    quest = fields[0].GetUInt32(); // Quest assign
                    dist = fields[1].GetFloat(); // Distance assign
                }
                else // If query is empty.. set to 0
                    quest = 0;
            }

            void MoveInLineOfSight(Unit* who)
            {
                Player* pl = who->ToPlayer(); // Player searching

                if (!pl) // If player exist
                    return;

                if (me->GetDistance(who) > dist) // limiting distance
                    return;

                Quest const* qInfo = sObjectMgr->GetQuestTemplate(quest); // Quest Gather

                if (!qInfo) // If quest exist
                    return;

                if (qInfo->HasFlag(QUEST_FLAGS_EXPLORATION) && qInfo->HasFlag(QUEST_TRINITY_FLAGS_EXPLORATION_OR_EVENT)) // if Quest Has QuestFlag = QUEST_FLAGS_EXPLORATION and SpecialFlag = QUEST_TRINITY_FLAGS_EXPLORATION_OR_EVENT
                {
                    if (qInfo && pl->GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE) // If Quest Status = QUEST_STATUS_INCOMPLETE
                        pl->CompleteQuest(quest); // Complete Quest
                }
            }

            void UpdateAI(const uint32 diff){} // not used now
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_areatrigger_completerAI(c);
        }
};


class npc_fac_race_changer : public CreatureScript
{
    public:
        npc_fac_race_changer(): CreatureScript("npc_fac_race_changer") {}

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (!pPlayer->HasAtLoginFlag(AT_LOGIN_CHANGE_RACE))
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I would like to change the race please.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

            if (!pPlayer->HasAtLoginFlag(AT_LOGIN_CHANGE_FACTION))
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "I would like to change the faction please.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(1,pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*sender*/, uint32 uiAction)
        {
            switch (uiAction)
            {
                case GOSSIP_ACTION_INFO_DEF+1:
                    pPlayer->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                    ChatHandler(pPlayer).PSendSysMessage("After next login you'll have the icon to race change next to your character.");
                    break;
                case GOSSIP_ACTION_INFO_DEF+2:
                    pPlayer->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                    ChatHandler(pPlayer).PSendSysMessage("After next login you'll have the icon to faction change next to your character.");
                    break;
            }

            pPlayer->CLOSE_GOSSIP_MENU();
            return true;
        }
};

void AddSC_npcs_special()
{
    new npc_air_force_bots;
    new npc_lunaclaw_spirit;
    new npc_chicken_cluck;
    new npc_dancing_flames;
    new npc_doctor;
    new npc_injured_patient;
    new npc_garments_of_quests;
    new npc_guardian;
    new npc_kingdom_of_dalaran_quests;
    new npc_mount_vendor;
    new npc_rogue_trainer;
    new npc_sayge;
    new npc_steam_tonk;
    new npc_tonk_mine;
    new npc_winter_reveler;
    new npc_brewfest_reveler;
    new npc_snake_trap;
    new npc_mirror_image;
    new npc_ebon_gargoyle;
    new npc_lightwell;
    new mob_mojo;
    new npc_training_dummy;
    new npc_shadowfiend;
    new npc_wormhole;
    new npc_pet_trainer;
    new npc_locksmith;
    new npc_tabard_vendor;
    new npc_experience;
    new npc_outdoor_deathwing_flight;
    new guardian_of_ancient_kings_holy;
    new guardian_of_ancient_kings_prot;
    new guardian_of_ancient_kings_ret;
    new npc_title_restorer;
    new npc_tail_receipe_giver;
    new npc_thrall_maelstrom;
    new quest_trigger;
    new npc_ring_of_frost;
    new npc_reforger;
    new npc_unmuter;
    new npc_flame_orb;
    new npc_power_word_barrier;
    new npc_jailer;
    new boss_event_jarmila;
    new boss_event_jarmila_pet;
    new npc_odevzdavac;
    new npc_areatrigger_completer;
    new npc_fac_race_changer;
}
