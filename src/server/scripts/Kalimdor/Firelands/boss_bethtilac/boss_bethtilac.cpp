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

/*
 * implementation of AI for boss Beth'tilac in Firelands
 * based on script in Deadly boss mods addon
 * and http://www.icy-veins.com/beth-tilac-detailed-strategy-wow
 */


#include "ScriptPCH.h"
#include <math.h>
#include <list>
#include "../firelands.h"
#include "Spell.h"

#include "boss_bethtilac_data.h"
#include "boss_bethtilac_spiderAI.h"


namespace Bethtilac
{


// Beth'tilac AI class declaration


enum BethtilacPhases    // needed to be here because it is used as function parameter
{
    PHASE_IDLE,
    PHASE_TRANSFER_1,
    PHASE_1,
    PHASE_TRANSFER_2,
    PHASE_2
};

#define MIDDLE_X 60.5f
#define MIDDLE_Y 387.0f
#define MIDDLE_Z 74.05f


const Position spawnPoints[3] = // Spiderling spawn points on 25 man difficulty
{
    {56.0f,426.0f,74.05f,4.3f}, // LEFT
    {105.0f,368.0f,76.5f,3.5f}, // MIDDLE
    {37.0f,327.0f,76.5f,1.4f}   // RIGHT
};

class boss_bethtilacAI: public SpiderAI
{
public:
    explicit boss_bethtilacAI(Creature *creature);
    virtual ~boss_bethtilacAI();

private:
    // virtual method overrides
    void Reset();
    void EnterCombat(Unit *who);
    bool UpdateVictim();
    void DamageTaken(Unit *attacker, uint32 &damage);
    void EnterEvadeMode();
    void KilledUnit(Unit *victim);
    void JustDied(Unit *killer);
    void UpdateAI(const uint32 diff);
    void DoAction(const int32 event);
    void MovementInform(uint32 type, uint32 id);
    //void SummonedCreatureDespawn(Creature *creature);
    void AttackStart(Unit *victim);

    // attributes
    BethtilacPhases phase, oldPhase;
    bool devastationEnabled;    // Smoldering devastation is disabled for a while after cast to avoid duplicate casts
    bool combatCheckEnabled;
    int devastationCounter;
    int spinnerCounter;         // number of the vawe of spinners
    uint32 broodlingTimer;

    // methods
    void SetPhase(BethtilacPhases newPhase);
    void EnterPhase(BethtilacPhases newPhase);
    void ScheduleEventsForPhase(BethtilacPhases phase);

    bool IsInTransfer();
    void ResetPower();
    void DepletePower();

    void DoSmolderingDevastation();

    void ShowWarnText(const char *text);    // show warning text in the client screen (checked by videos)

    GameObject *FindDoor();
    void LockDoor();
    void UnlockDoor();

    // spawns
    void SummonDrone();
    void SummonSpinners(bool withWarn);
    void SummonSpiderlings();
};



// Beth'tilac script loader
class boss_bethtilac: public CreatureScript
{
public:
    boss_bethtilac() : CreatureScript("boss_bethtilac") {}
    CreatureAI *GetAI(Creature *creature) const
    {
        return new boss_bethtilacAI(creature);
    }
};



//////////////////////////////////////////////////////////////////////////
// Beth'tilac implementation


enum BethtilacSpells
{
    // phase 1
    SPELL_SMOLDERING_DEVASTATION = 99052,
    SPELL_EMBER_FLARE = 98934,
    SPELL_VENOM_RAIN = 99333,
    SPELL_METEOR_BURN = 99133,
    // phase 2
    SPELL_FRENZY = 99497,
    SPELL_EMBER_FLARE_2 = 99859,
    SPELL_WIDOW_KISS = 99476,

    // summons
    SPELL_SUMMON_SPINNER = 98872,

    //Engorged Broodling ability
    SPELL_VOLATILE_BURST = 99990,
    SPELL_VOLATILE_POISON = 101136,
};

enum BethtilacEvents
{
    POWER_DECAY = 0,        // decay 100 energy each second

    // combat check
    COMBAT_CHECK_ENABLE,

    // phase 1
    TRANSFER1_START,
    SD_ENABLE,            // Smoldering Devastation enabled
    SP_EMBER_FLARE,       // cast Ember Flare
    SP_VENOM_RAIN,        // cast Venom Rain (instant) if no one is in range
    SP_METEOR_BURN,       // cast Meteor Burn
    SUMMON_FIRST_DRONE,   // first Cinderweb Drone is spawned earlier than others
    SUMMON_DRONE,
    SUMMON_SPINNERS,      // 3 waves per 1 devastation
    SUMMON_FIRST_SPIDERLINGS,
    SUMMON_SPIDERLINGS,

    // phase 2
    END_OF_PHASE_1,       // phase 1 ended
    SP_FRENZY,
    SP_EMBER_FLARE_2,
    SP_WIDOW_KISS,
};


static const int MAX_DEVASTATION_COUNT = 3;



boss_bethtilacAI::boss_bethtilacAI(Creature *creature)
    : SpiderAI(creature)
{
    phase = PHASE_IDLE;
    devastationEnabled = true;
    devastationCounter = 0;
    spinnerCounter = 0;
}


boss_bethtilacAI::~boss_bethtilacAI()
{
    //if (summonCombatChecker)
    //    summonCombatChecker->UnSummon();
}


void boss_bethtilacAI::Reset()
{
    if (instance)
        instance->SetData(TYPE_BETHTILAC, NOT_STARTED);

    DebugOutput("R E S E T");

    ResetPower();
    ClearTimers();

    me->SetHealth(me->GetMaxHealth());
    phase = PHASE_IDLE;

    // teleport to starting position and set "flying"
    me->SetFlying(true);        // don't use hover, it would break the visual effect
    me->StopMoving();
    Position start = me->GetHomePosition();
    me->NearTeleportTo(start.GetPositionX(), start.GetPositionY(), start.GetPositionZ(), start.GetOrientation());
    me->GetMotionMaster()->MovePoint(0, start);

    // summon the filament
    SummonFilament();

    // allow the door to be open
    UnlockDoor();
}


void boss_bethtilacAI::EnterCombat(Unit *who)
{
    if (phase != PHASE_IDLE)
        return;

    LockDoor();

    if (instance)
        instance->SetData(TYPE_BETHTILAC, IN_PROGRESS);

    DoZoneInCombat();

    devastationEnabled = true;
    devastationCounter = 0;

    spinnerCounter = 0;

    combatCheckEnabled = true;
    broodlingTimer = 20000;

    EnterPhase(PHASE_TRANSFER_1);
}


bool boss_bethtilacAI::UpdateVictim()
{
    if (phase == PHASE_IDLE)
        return false;

    if (IsInTransfer() || ScriptedAI::UpdateVictim())
    {
        combatCheckEnabled = true;
        return true;
    }

    if (phase != PHASE_1 || !me->isInCombat())
        return false;

    // in phase 1 while there are enemies below the web
    me->AttackStop();
    return true;
}


void boss_bethtilacAI::DamageTaken(Unit *attacker, uint32 &damage)
{
    if (phase == PHASE_IDLE)
    {
        EnterCombat(attacker);
    }
}


void boss_bethtilacAI::EnterEvadeMode()
{
    if (!combatCheckEnabled)
        return;

    // don't evade when players are on another floor
      // search for all players, including unattackable and hidden
    {
        using namespace Trinity;
        using namespace std;
        typedef AnyPlayerInObjectRangeCheck Check;

        float radius = 100.0f;

        list<Player*> players;
        Check checker(me, radius);
        PlayerListSearcher<Check> searcher(me, players, checker);
        me->VisitNearbyWorldObject(radius, searcher);
        for (list<Player*>::iterator it = players.begin(); it != players.end(); it++)
        {
            Player *player = *it;
            if (player->IsInWorld() && player->isAlive() && !player->isGameMaster())
                return;
        }
    }


    // really evade
    ScriptedAI::EnterEvadeMode();

    if (instance)
        instance->SetData(TYPE_BETHTILAC, FAIL);

    UnlockDoor();
}


void boss_bethtilacAI::KilledUnit(Unit *victim)
{
}


void boss_bethtilacAI::JustDied(Unit *killer)
{
    SpiderAI::JustDied(killer);
    me->RemoveAllAuras();

    if (instance)
        instance->SetData(TYPE_BETHTILAC, DONE);

    UnlockDoor();
}


void boss_bethtilacAI::UpdateAI(const uint32 diff)
{
    if (!UpdateVictim())
    {
        if (instance && instance->GetData(TYPE_BETHTILAC) == IN_PROGRESS)
            EnterEvadeMode();
        else if (me->isInCombat())
            SpiderAI::EnterEvadeMode();
        return;
    }

    if (phase == PHASE_1 && me->GetPositionZ() < webZPosition)
    {
        me->AttackStop();
        me->StopMoving();
        me->DeleteThreatList();
        me->NearTeleportTo(me->GetPositionX() + 1.0f, me->GetPositionY(), webZPosition + 5.0f, me->GetOrientation());
    }

    if (!IsInTransfer() && !me->IsNonMeleeSpellCasted(false))
        if (Unit *pVictim = me->getVictim())
        {
            if (me->IsWithinMeleeRange(pVictim))
                DoMeleeAttackIfReady();
            else
                me->GetMotionMaster()->MoveChase(pVictim);
        }


    if (phase == PHASE_1 && IsHeroic())
    {
        if (broodlingTimer <= diff)
        {
            for (uint8 i = 0; i < 3; i++)
            {
                Creature * broodling = me->SummonCreature(NPC_ENGORGED_BROODLING, spawnPoints[i].GetPositionX(), spawnPoints[i].GetPositionY(), spawnPoints[i].GetPositionZ(), spawnPoints[i].GetOrientation(),
                    TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
                if (broodling)
                    broodling->SetSpeed(MOVE_RUN, 1.6f, true); // Guessing
            }
            broodlingTimer = urand(7000, 12000);
        }
        else broodlingTimer -= diff;
    }

    UpdateTimers(diff);

    if (phase != oldPhase)
        EnterPhase(phase);
}


void boss_bethtilacAI::SetPhase(BethtilacPhases newPhase)
{
    oldPhase = phase;
    phase = newPhase;
}


void boss_bethtilacAI::EnterPhase(BethtilacPhases newPhase)
{
    if (newPhase != PHASE_1)
        ClearTimers();

    phase = newPhase;
    oldPhase = newPhase;

    ScheduleEventsForPhase(newPhase);

    switch (newPhase)
    {
        case PHASE_IDLE:
            ScriptedAI::EnterEvadeMode();
            break;

        case PHASE_TRANSFER_1:
        {
            DebugOutput("phase transfer 1");
            // anim and movement up
            me->PlayOneShotAnimKit(ANIM_KIT_EMERGE);
            AddTimer(TRANSFER1_START, 1000, false);
            break;
        }

        case PHASE_1:
        {
            DebugOutput("phase 1");

            me->AttackStop();
            me->DeleteThreatList();
            me->SetReactState(REACT_AGGRESSIVE);

            UnSummonFilament();

            me->SetInCombatWithZone();
            DoAction(SP_VENOM_RAIN);
            break;
        }

        case PHASE_TRANSFER_2:
        {
            DebugOutput("phase transfer 2");

            me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 1.0f, me->GetOrientation());
            SummonFilament();

            MoveToGround(MOVE_POINT_DOWN);
            break;
        }

        case PHASE_2:
        {
            DebugOutput("phase 2");

            me->AttackStop();
            me->DeleteThreatList();
            me->SetReactState(REACT_AGGRESSIVE);

            UnSummonFilament();

            DoZoneInCombat();

            break;
        }
    }
}


void boss_bethtilacAI::ScheduleEventsForPhase(BethtilacPhases phase)
{
    if (phase != PHASE_IDLE && phase != PHASE_1)
        AddTimer(POWER_DECAY, 1000, true);

    switch (phase)
    {
        case PHASE_TRANSFER_1:
            // timers for phase 1 should start in the moment of entering to combat (except "auto-cast" spells)
            AddTimer(SUMMON_FIRST_DRONE, 45000, false);
            AddTimer(SUMMON_SPINNERS, 12000, false);
            AddTimer(SUMMON_FIRST_SPIDERLINGS, 12500, false);
            break;
        case PHASE_1:
            AddTimer(SP_EMBER_FLARE, 6000, true);
            AddTimer(SP_VENOM_RAIN, 2500, true);
            AddTimer(SP_METEOR_BURN, 16000, true);
            break;
        case PHASE_TRANSFER_2:
            break;
        case PHASE_2:
            AddTimer(SP_FRENZY, 5000, true);
            AddTimer(SP_EMBER_FLARE_2, 6000, true);
            AddTimer(SP_WIDOW_KISS, 32000, true);
            break;
        default:
            break;
    }
}


void boss_bethtilacAI::ResetPower()
{
    SetMaxPower(8200);
    SetPower(8200);
    me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
}


bool boss_bethtilacAI::IsInTransfer()
{
    return phase == PHASE_TRANSFER_1 || phase == PHASE_TRANSFER_2;
}


void boss_bethtilacAI::DoAction(const int32 event)
{
    switch (event)
    {
        case TRANSFER1_START:
            MoveToFilament(MOVE_POINT_UP);
            break;

        case POWER_DECAY:
            DepletePower();
            if (phase == PHASE_1 && GetPower() == 0)
                DoSmolderingDevastation();
            break;

        case COMBAT_CHECK_ENABLE:
            combatCheckEnabled = true;
            break;

        case SD_ENABLE:
            devastationEnabled = true;
            break;

        case SP_VENOM_RAIN:
            if (!me->getVictim() && !me->IsNonMeleeSpellCasted(false))
                me->CastSpell((Unit*)NULL, SPELL_VENOM_RAIN, false);
            break;

        case SP_EMBER_FLARE:
            if (me->getVictim() && !me->IsNonMeleeSpellCasted(false))
                me->CastSpell((Unit*)NULL, SPELL_EMBER_FLARE, false);
            break;

        case SP_METEOR_BURN:
            if (!me->IsNonMeleeSpellCasted(false) && me->getVictim())
            {
                DebugOutput("casting Meteor Burn");
                me->CastCustomSpell(SPELL_METEOR_BURN, SPELLVALUE_MAX_TARGETS, 1, me, false);
            }
            break;

        case SUMMON_FIRST_DRONE:
            AddTimer(SUMMON_DRONE, 60000, true);
        case SUMMON_DRONE:
            SummonDrone();
            break;

        case SUMMON_SPINNERS:
            if (++spinnerCounter < 3)
                AddTimer(SUMMON_SPINNERS, 10000, false);

            SummonSpinners(spinnerCounter == 1);
            break;

        case SUMMON_FIRST_SPIDERLINGS:
            AddTimer(SUMMON_SPIDERLINGS, 30000, true);
        case SUMMON_SPIDERLINGS:
            SummonSpiderlings();
            break;

        case END_OF_PHASE_1:
            SetPhase(PHASE_TRANSFER_2);
            break;

        case SP_FRENZY:
            me->CastSpell(me, SPELL_FRENZY, false);
            break;

        case SP_EMBER_FLARE_2:
            me->CastSpell((Unit*)NULL, SPELL_EMBER_FLARE_2, false);
            break;

        case SP_WIDOW_KISS:
            me->CastSpell(me->getVictim(), SPELL_WIDOW_KISS, false);
            break;

        default:
            SpiderAI::DoAction(event);
            break;
    }
}


void boss_bethtilacAI::MovementInform(uint32 type, uint32 id)
{
    ScriptedAI::MovementInform(type, id);

    switch (id)
    {
        case MOVE_POINT_UP:
        {
            if (type == POINT_MOTION_TYPE)
            {
                SetPhase(PHASE_1);
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), webZPosition + 1.0f, me->GetOrientation(), false);
                me->SetFlying(false);
                DoZoneInCombat();
            }

            break;
        }

        case MOVE_POINT_DOWN:
        {
            if (type == POINT_MOTION_TYPE)
            {
                SetPhase(PHASE_2);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat();
                me->SetFlying(false);
                me->SendMovementFlagUpdate();
            }
            break;
        }
    }
}


void boss_bethtilacAI::AttackStart(Unit *victim)
{
    if (phase != PHASE_IDLE && !IsInTransfer() && me->canAttack(victim, false))
    {
        if ((phase != PHASE_1 && victim->GetPositionZ() < webZPosition - 5) ||
            (phase == PHASE_1 && victim->GetPositionZ() >= webZPosition - 5))
        {
            ScriptedAI::AttackStart(victim);
        }
    }
}


void boss_bethtilacAI::DepletePower()
{
    ModifyPower(-100);
}


void boss_bethtilacAI::DoSmolderingDevastation()
{
    if (devastationEnabled)
    {
        ShowWarnText("Beth'tilac smoldering body begins to flicker and combust!");

        devastationEnabled = false;          // disable it temporarily to avoid double cast
        AddTimer(SD_ENABLE, 10000, false);   // re-enable it after 10 seconds

        me->CastSpell(me, SPELL_SMOLDERING_DEVASTATION, false);    // has cast time - can't be flagged as triggered

        std::list<Creature*> webRips;
        GetCreatureListWithEntryInGrid(webRips, me, NPC_WEB_RIP, 200.0f);

        for (std::list<Creature*>::iterator iter = webRips.begin(); iter != webRips.end(); ++iter)
            (*iter)->ForcedDespawn(8000);

        devastationCounter++;   // increase counter - after third one transfer to next phase

        if (devastationCounter == MAX_DEVASTATION_COUNT)
        {
            AddTimer(END_OF_PHASE_1, me->GetCurrentSpellCastTime(SPELL_SMOLDERING_DEVASTATION) + 200, false);
        }
        else
        {
            // next Cinderweb Spinners in 15 seconds after start of cast
            AddTimer(SUMMON_SPINNERS, 15000, false);
        }

        // enable next waves of spinners
        spinnerCounter = 0;
    }
}


void boss_bethtilacAI::ShowWarnText(const char *text)
{
    me->MonsterTextEmote(text, 0, true);
}


void boss_bethtilacAI::SummonDrone()
{
    if (devastationCounter >= MAX_DEVASTATION_COUNT)
        return;

    DebugOutput("summoning Cinderweb Drone");

    me->SummonCreature(NPC_CINDERWEB_DRONE, 46.0f, 458.0f, 76.5f, 4.6f,
                       TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
}


void boss_bethtilacAI::SummonSpinners(bool withWarn)
{
    if (devastationCounter >= MAX_DEVASTATION_COUNT)
        return;

    if (withWarn)
    {
        ShowWarnText("Cinderweb Spinners dangle from above!");
        DebugOutput("summoning Cinderweb Spinners");
    }

    int spinners = RAID_MODE(2, 5, 2, 5);
    for (int i = 0; i < spinners; i++)
        me->CastSpell(me, SPELL_SUMMON_SPINNER, true);
}


void boss_bethtilacAI::SummonSpiderlings()
{
    if (devastationCounter >= MAX_DEVASTATION_COUNT)
        return;

    ShowWarnText("Spiderlings have been roused from their nest!");
    DebugOutput("summoning Cinderweb Spiderlings");


    if (Is25ManRaid()) // In 25 man, group of 3-4 spiderlings are spawned at three specific locations
    {
        for (uint8 i = 0; i < 3; i++)
        {
            uint8 spidersNum = urand(3, 4);

            for (uint8 j = 0; j < spidersNum; j++)
            {
                float x, y, z,o;
                // spawn them at 4 yards distance 
                x = MIDDLE_X + cos(me->GetAngle(spawnPoints[i].GetPositionX(), spawnPoints[i].GetPositionY())) * (me->GetDistance2d(spawnPoints[i].GetPositionX(), spawnPoints[i].GetPositionY()) + (4*j));
                y = MIDDLE_Y + sin(me->GetAngle(spawnPoints[i].GetPositionX(), spawnPoints[i].GetPositionY())) * (me->GetDistance2d(spawnPoints[i].GetPositionX(), spawnPoints[i].GetPositionY()) + (4*j));
                z = me->GetMap()->GetHeight2(x, y, 80.0f);
                o = spawnPoints[i].GetOrientation();

                Creature * spiderLing = me->SummonCreature(NPC_CINDERWEB_SPIDERLING, x, y, z, o, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

                if (spiderLing)
                    spiderLing->SetSpeed(MOVE_RUN, 0.6f, true);
            }
        }
    }
    else // 10 man
    {
        uint8 randPos = urand(0, 2);

        for (uint8 j = 0; j < 10; j++) // Spawn 10 broodlings on one random position
        {
            float x, y, z, o;
            // spawn them at 2 yards distance 
            x = MIDDLE_X + cos(me->GetAngle(spawnPoints[randPos].GetPositionX(), spawnPoints[randPos].GetPositionY())) * (me->GetDistance2d(spawnPoints[randPos].GetPositionX(), spawnPoints[randPos].GetPositionY()) + (2 * j));
            y = MIDDLE_Y + sin(me->GetAngle(spawnPoints[randPos].GetPositionX(), spawnPoints[randPos].GetPositionY())) * (me->GetDistance2d(spawnPoints[randPos].GetPositionX(), spawnPoints[randPos].GetPositionY()) + (2 * j));
            z = me->GetMap()->GetHeight2(x, y, 80.0f);
            o = spawnPoints[randPos].GetOrientation();

            Creature * spiderLing = me->SummonCreature(NPC_CINDERWEB_SPIDERLING, x, y, z, o, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);

            if (spiderLing)
                spiderLing->SetSpeed(MOVE_RUN, 0.5f, true);
        }
    }

}


GameObject *boss_bethtilacAI::FindDoor()
{
    return me->FindNearestGameObject(208877, 100);
}


void boss_bethtilacAI::LockDoor()
{
    if (GameObject *goDoor = FindDoor())
    {
        goDoor->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
        goDoor->SetGoState(GO_STATE_READY);     // close door
    }
}


void boss_bethtilacAI::UnlockDoor()
{
    if (GameObject *goDoor = FindDoor())
    {
        goDoor->SetGoState(GO_STATE_ACTIVE);    // open door
        goDoor->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
    }
}



class boss_bethilac_engorged_broodling : public CreatureScript
{
public:
    boss_bethilac_engorged_broodling() : CreatureScript("boss_bethilac_engorged_broodling") {};

    struct boss_bethilac_engorged_broodlingAI : public ScriptedAI
    {
        boss_bethilac_engorged_broodlingAI(Creature* c) : ScriptedAI(c)
        {
            instance = me->GetInstanceScript();
        }

        Player* SelectPlayerOnGround()
        {
            if (!instance)
                return NULL;

            Map::PlayerList const& plList = instance->instance->GetPlayers();

            if (plList.isEmpty())
                return NULL;

            std::list<Player*> groundPlayers;
            groundPlayers.clear();

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * pl = itr->getSource())
                {
                    if (pl->GetPositionZ() < (MIDDLE_Z + 10.0f))
                        groundPlayers.push_back(pl);
                }
            }

            if (groundPlayers.empty())
                return NULL;

            std::list<Player*>::iterator j = groundPlayers.begin();
            advance(j, rand() % groundPlayers.size()); // Pick random target

            if ((*j) && (*j)->IsInWorld())
            {
                return (*j);
            }
            return NULL;
        }

        InstanceScript * instance;
        uint32 checkTimer;

        void Reset()
        {
            checkTimer = 2000;
            me->SetInCombatWithZone();
            if (Player * pl = SelectPlayerOnGround())
            {
                me->AddThreat(pl, 500000.0f);
                me->GetMotionMaster()->MoveChase(pl);
            }
        }

        void MoveInLineOfSight(Unit* who)
        {
            /*if (who && who->GetTypeId() == TYPEID_PLAYER && me->IsWithinMeleeRange(who))
            {
                me->CastSpell(me, SPELL_VOLATILE_BURST, true); // This will also kill broodling
            }*/
            ScriptedAI::MoveInLineOfSight(who);
        }

        Player * GetPlayerInMeleeRange()
        {
            using namespace Trinity;
            using namespace std;
            typedef AnyPlayerInObjectRangeCheck Check;

            float radius = 10.0f;

            list<Player*> players;
            Check checker(me, radius);
            PlayerListSearcher<Check> searcher(me, players, checker);
            me->VisitNearbyWorldObject(radius, searcher);
            for (list<Player*>::iterator it = players.begin(); it != players.end(); it++)
            {
                Player *player = *it;
                if (player->IsInWorld() && player->isAlive() && !player->isGameMaster() && player->GetExactDist2d(me) < 6.0f)
                    return player;
            }

            return NULL;
        }

        void JustDied(Unit* /*killer*/)
        {
            //Need to summon custom NPC for volatile poison after death
            me->SummonCreature(537450,me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN,20000);
        }

        void UpdateAI(const uint32 diff)
        {
            if (instance && instance->GetData(TYPE_BETHTILAC) != IN_PROGRESS)
                me->ForcedDespawn();

            if (!UpdateVictim())
                return;

            if (checkTimer <= diff)
            {
                if (GetPlayerInMeleeRange())
                {
                    me->CastSpell(me, SPELL_VOLATILE_BURST, true); // This will also kill broodling
                    return;
                }

                if (me->GetPositionZ() >= MIDDLE_Z + 10.0f) // Don't chase victims at web
                {
                    if (Player * pl = SelectPlayerOnGround()) // If so select new target
                    {
                        DoResetThreat();
                        me->SetInCombatWithZone();
                        me->AddThreat(pl, 500000.0f);
                        me->GetMotionMaster()->Clear(false);
                        me->GetMotionMaster()->MoveChase(pl);
                    }
                }
                checkTimer = 100;
            }
            else checkTimer -= diff;

            //DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_bethilac_engorged_broodlingAI(c);
    }
};


class boss_bethilac_volatile_poison : public CreatureScript
{
public:
    boss_bethilac_volatile_poison() : CreatureScript("boss_bethilac_volatile_poison") {};

    struct boss_bethilac_volatile_poisonAI : public ScriptedAI
    {
        boss_bethilac_volatile_poisonAI(Creature* c) : ScriptedAI(c)
        {
            me->setFaction(14);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            delayTimer = 200;
        }

        uint32 delayTimer;

        void Reset() { }
        void EnterEvadeMode() { }

        void UpdateAI(const uint32 diff)
        {
            if (delayTimer <= diff)
            {
                me->CastSpell(me, SPELL_VOLATILE_POISON, true);
                delayTimer = 60000;
            }
            else delayTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new boss_bethilac_volatile_poisonAI(c);
    }
};


void load_boss_Bethtilac()
{
    new boss_bethtilac();
    new boss_bethilac_engorged_broodling();
    new boss_bethilac_volatile_poison();
};

}   // end of namespace
