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
#include "well_of_eternity.h"
#include "MapManager.h"

#define MAX_VEHICLES 5
#define MINUTE_IN_MILIS 60000

const Position vehicleSpawnPos [MAX_VEHICLES] = 
{
    {3471.37f,-5279.54f,229.94f,1.76f},
    {3461.31f,-5287.13f,233.28f,1.52f},
    {3453.41f,-5281.62f,229.94f,1.15f},
    {3442.69f,-5282.94f,232.88f,1.30f},
    {3436.57f,-5276.92f,229.94f,1.21f}
};

const Position vehicleMiddlePos [MAX_VEHICLES] = 
{
    {3440.00f,-5381.0f,248.92f,4.44f},
    {3433.69f,-5380.0f,252.00f,4.50f},
    {3425.00f,-5380.0f,248.22f,4.55f},
    {3416.70f,-5379.0f,250.25f,4.60f},
    {3408.00f,-5378.1f,248.30f,4.70f}
};

const Position vehicleEndPos [MAX_VEHICLES] = 
{
    {3257.50f,-5533.58f,85.80f,3.50f},
    {3255.93f,-5523.60f,94.30f,3.70f},
    {3251.72f,-5517.26f,85.80f,3.54f},
    {3250.82f,-5507.04f,92.50f,3.64f},
    {3244.67f,-5498.17f,85.80f,3.64f}
};

const Position landingPos = { 3132.86f, -5559.66f, 18.05f, 3.68f };
const Position NeltharionSpawnPos = {3565.99f,-5611.27f,337.80f,2.24f};
const Position unknownEvilPos = { 3399.0f, -5467.0f, 113.0f, 0.0f };

const Position demonWellSummonPos1 = { 3476.0f, -5492.0f, 5.0f, 6.00f };
const Position demonWellSummonPos2 = { 3338.0f, -5441.0f, 5.0f, 3.56f };

enum entries
{
    // Aspects
    ENTRY_YSERA_ASPECT = 55393,
    ENTRY_ALEXSTRASZA_ASPECT = 55394,
    ENTRY_SORIDORMI_ASPECT = 55395,
    ENTRY_NELTHARION_ASPECT = 55400,
    // Dragon soul
    ENTRY_DRAGON_SOUL = 55078,
    // Another NPCs
    ENTRY_UNKNOWN_EVIL = 57201,
    ENTRY_BRONZE_DRAKE_VEHICLE = 57107,
    ENTRY_DEATHWING_VEHICLE_PASSENGER = 51358,
    ENTRY_SHADOWBAT_DRAKE_KILLER = 57458,
    ENTRY_WELL_DOOMGUARD_DEVASTATOR = 57410
};

typedef struct quote_event
{
    const uint32 nextEventTime;
    const uint32 entry;
    const char * yellQuote;
    const uint32 soundID;
}QUOTE_EVENTS;

#define MAX_QUOTES 14

QUOTE_EVENTS quoteEvents [MAX_QUOTES] =
{
    {11000, ENTRY_BRONZE_DRAKE_VEHICLE, "Sister, look! The artifact is surrounded by an aura of darkness!", 26031},
    {11000, ENTRY_BRONZE_DRAKE_VEHICLE, "The aspects!", 26033},
    {19000, ENTRY_ALEXSTRASZA_ASPECT, "Come sisters, it is time to end this. We will cleanse the Dragon Soul of the Old Gods‘ dark influence!", 26508},
    {10000, ENTRY_YSERA_ASPECT, "It is too powerful!", 26150},
    {5000, ENTRY_NELTHARION_ASPECT, "How DARE you touch my creation?! It is mine...mine...MINE!", 26107},
    {7000, ENTRY_ALEXSTRASZA_ASPECT, "Neltharion! What have you done? You have doomed us all to the madness of the Old Gods!", 26509},
    {3000, ENTRY_NELTHARION_ASPECT, "IT IS MINE!!", 26108},
    {6000, ENTRY_YSERA_ASPECT, "He is lost to us, sister. He is Neltharion no longer.", 26151},
    {5000, ENTRY_ALEXSTRASZA_ASPECT, "The power of the Soul is tearing him apart!", 26510},
    {4000, ENTRY_UNKNOWN_EVIL, "Nooooo...away...", 26110},
    {12000, ENTRY_NELTHARION_ASPECT, "I WILL NOT BE DENIED!", 26109},
    {6000, ENTRY_UNKNOWN_EVIL, "AWAY!", 26111},
    {5000, ENTRY_BRONZE_DRAKE_VEHICLE, "The Old Gods protect the Soul!", 26034},
    {MAX_TIMER, ENTRY_BRONZE_DRAKE_VEHICLE, "The link to the portal must be broken! Quickly, to Stormrage!", 26036}
};

SimpleQuote drakeSpawnQuote = {26030, "It is them!" };
SimpleQuote drakeIntroQuote = {26032, "Heroes! We have been sent by Nozdormu! Quickly, on our backs, we must get you to the Dragon Soul!" };

enum spells
{
    SPELL_THE_DRAGON_SOUL_COSMETIC = 102845,
    SPELL_DRAGOUN_SOUL_COSMETIC_CHARGED = 110489,
    SPELL_TIME_TELEPORT_DUMMY_EFFECT = 107493,
    SPELL_FLYING_ANIM_KIT_AURA = 98723,
    SPELL_DRAGON_SOUL_EXPLOSION =108826
};

enum beamSpells
{
    SPELL_EVIL_DRAGON_SOUL_BEAM = 108759,
    SPELL_GOOD_ALEX_BEAM = 107618,
    SPELL_GOOD_YSERA_BEAM = 108615,
    SPELL_GOOD_SORI_BEAM = 108616 
};

enum drakeData
{
    DATA_DRAKE_NUMBER,
};

static void GetLinePoint(float &x, float &y, float distance, float angle)
{
    x = x + cos(angle)*distance;
    y = y + sin(angle)*distance;
}

#define YSERA_SPAWN_ANGLE 4.0f
#define ALEX_SPAWN_ANGLE 4.8f
#define SORI_SPAWN_ANGLE 3.4f
#define ASPECT_SPAWN_DISTANCE 180.0f
#define ASPECT_ARRIVE_DISTANCE 130.0f

class npc_the_dragon_soul_woe : public CreatureScript
{
public:
    npc_the_dragon_soul_woe() : CreatureScript("npc_the_dragon_soul_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_the_dragon_soul_woeAI(creature);
    }

    struct npc_the_dragon_soul_woeAI : public ScriptedAI
    {
        npc_the_dragon_soul_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            summonDemonsTimer = 2000;
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_THE_DRAGON_SOUL_COSMETIC, true);
            eventStarted = false;
            firstDrakeArrived = false;
            quoteStep = 0;
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case ENTRY_UNKNOWN_EVIL:
                    summon->setFaction(35);
                    summon->SetVisible(false);
                    break;
                case ENTRY_BRONZE_DRAKE_VEHICLE:
                    drakeList.push_back(summon->GetGUID());
                    break;
                case ENTRY_NELTHARION_ASPECT:
                {
                    float x = me->GetPositionX();
                    float y = me->GetPositionY();
                    float z = me->GetPositionZ() + 15.0f;
                    float distance = 55.0f;

                    GetLinePoint(x, y, distance,me->GetAngle(summon));
                    summon->GetMotionMaster()->MovePoint(0, x, y, z);
                    break;
                }
                case ENTRY_WELL_DOOMGUARD_DEVASTATOR:
                {
                    summon->CastSpell(summon, SPELL_FLYING_ANIM_KIT_AURA, true);
                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    float x = summon->GetPositionX();
                    float y = summon->GetPositionY();
                    float z = summon->GetPositionZ() + 40.0f;
                    float distance = 100.0f;
                    GetLinePoint(x, y, distance, summon->GetOrientation());
                    summon->GetMotionMaster()->MovePoint(0, x, y, z);
                    break;
                }
            }
        }

        void StartEvent()
        {
            quoteTimer = 1000;
            beamsTimer = 29000;
            textEmoteTimer = 37000;
            eventStarted = true;
        }

        void SpawnDrakeVehicles()
        {
            for (uint32 i = 0; i < MAX_VEHICLES; i++)
            {
                if (Creature * pDrake = me->SummonCreature(ENTRY_BRONZE_DRAKE_VEHICLE,vehicleSpawnPos[i]))
                {
                    pDrake->AI()->SetData(DATA_DRAKE_NUMBER, i);
                    pDrake->CastSpell(pDrake, SPELL_TIME_TELEPORT_DUMMY_EFFECT, true);
                    if (i == 2)
                    {
                        pDrake->MonsterYell(drakeSpawnQuote.text,0,0);
                        pDrake->PlayDirectSound(drakeSpawnQuote.soundID);
                    }
                }
            }
        }

        std::list<uint64> drakeList;
        bool firstDrakeArrived;
        bool eventStarted;
        InstanceScript * pInstance;
        // Generic quote stuff
        uint32 quoteTimer;
        uint32 quoteStep;
        // Other timers
        uint32 beamsTimer;
        uint32 textEmoteTimer;
        uint32 summonDemonsTimer;

        void Reset() override { drakeList.clear(); }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_SPAWN_DRAKE_VEHICLES)
            {
                SpawnDrakeVehicles();
            }
            else if (action == ACTION_BRONZE_DRAKE_ARRIVED && firstDrakeArrived == false)
            {
                firstDrakeArrived = true;
                StartEvent();
            }
        }

        void SpellHit(Unit *, const SpellEntry * spell) override
        {
            if (spell->Id == SPELL_GOOD_SORI_BEAM)
                me->AddAura(SPELL_DRAGOUN_SOUL_COSMETIC_CHARGED, me);
        }

        inline void ResetMyPosition(float &x, float &y)
        {
            x = me->GetPositionX();
            y = me->GetPositionY();
        }

        void HandleQuote()
        {
            float x = me->GetPositionX();
            float y = me->GetPositionY();
            float z = me->GetPositionZ();

            // Some special events
            switch (quoteStep)
            {
                case 1:
                {
                    GetLinePoint(x, y, ASPECT_SPAWN_DISTANCE,ALEX_SPAWN_ANGLE);
                    if (Creature * alexAspect = me->SummonCreature(ENTRY_ALEXSTRASZA_ASPECT,x,y,z, 1.96f, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS))
                    {
                        ResetMyPosition(x, y);
                        GetLinePoint(x, y, ASPECT_ARRIVE_DISTANCE,ALEX_SPAWN_ANGLE);
                        alexAspect->GetMotionMaster()->MovePoint(0, x, y, z);
                    }

                    GetLinePoint(x, y, ASPECT_SPAWN_DISTANCE - 10.0f,YSERA_SPAWN_ANGLE);
                    if (Creature * yseraAspect = me->SummonCreature(ENTRY_YSERA_ASPECT,x,y,z - 5.0f, 1.16f, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS))
                    {
                        ResetMyPosition(x, y);
                        GetLinePoint(x, y, ASPECT_ARRIVE_DISTANCE,YSERA_SPAWN_ANGLE);
                        yseraAspect->GetMotionMaster()->MovePoint(0, x, y, z);
                    }

                    GetLinePoint(x, y, ASPECT_SPAWN_DISTANCE - 25.0f,SORI_SPAWN_ANGLE);
                    if (Creature * soriAspect = me->SummonCreature(ENTRY_SORIDORMI_ASPECT,x,y,z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS))
                    {
                        ResetMyPosition(x, y);
                        GetLinePoint(x, y, ASPECT_ARRIVE_DISTANCE,SORI_SPAWN_ANGLE);
                        soriAspect->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                    break;
                }
                case 3:
                {
                    me->RemoveAura(SPELL_DRAGOUN_SOUL_COSMETIC_CHARGED);
                    for (uint32 entry = ENTRY_YSERA_ASPECT; entry <= ENTRY_SORIDORMI_ASPECT; entry++)
                    {
                        if (Creature * pAspect = me->FindNearestCreature(entry, me->GetMap()->GetVisibilityDistance(), true))
                            pAspect->InterruptNonMeleeSpells(false);
                    }
                    break;
                }
                case 4:
                {
                    me->SummonCreature(ENTRY_NELTHARION_ASPECT, NeltharionSpawnPos, TEMPSUMMON_TIMED_DESPAWN, 2 *MINUTE_IN_MILIS);
                    break;
                }
                case 9:
                {
                    me->SummonCreature(ENTRY_UNKNOWN_EVIL, unknownEvilPos, TEMPSUMMON_TIMED_DESPAWN, MINUTE_IN_MILIS);
                    break;
                }
                case 12:
                {
                    me->CastSpell(me, SPELL_DRAGON_SOUL_EXPLOSION, false);
                    for (std::list<uint64>::const_iterator it = drakeList.begin(); it != drakeList.end(); ++it)
                    {
                        if (Creature * pDrake = ObjectAccessor::GetCreature(*me,*it))
                            pDrake->AI()->DoAction(ACTION_VEHICLE_MOVE);
                    }
                    drakeList.clear();
                    break;
                }
                default:
                    break;
            }

            // Process current quote
            QUOTE_EVENTS qEvent = quoteEvents[quoteStep];

            if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,me->GetMap()->GetVisibilityDistance(),true))
            {
                pCreature->MonsterYell(qEvent.yellQuote, 0, 0,me->GetMap()->GetVisibilityDistance());
                pCreature->PlayDirectSound(qEvent.soundID);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (summonDemonsTimer <= diff)
            {
                if (quoteStep < 3) // Don't summon when cleansing well
                {
                    float x,y,z,o;
                    if (Creature * doomguardLeader = me->SummonCreature(ENTRY_WELL_DOOMGUARD_DEVASTATOR,demonWellSummonPos1,TEMPSUMMON_TIMED_DESPAWN,12000))
                    {
                        for (uint32 i = 0; i < 5; i++)
                        {
                            x = demonWellSummonPos1.GetPositionX();
                            y = demonWellSummonPos1.GetPositionY();
                            z = demonWellSummonPos1.GetPositionZ();
                            o = demonWellSummonPos1.GetOrientation() + (M_PI / 3.0f) * (i + 1);

                            GetLinePoint(x, y, 10.0f, MapManager::NormalizeOrientation(o));

                            me->SummonCreature(ENTRY_WELL_DOOMGUARD_DEVASTATOR, x, y, z, demonWellSummonPos1.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 12000);
                        }
                    }

                    if (Creature * doomguardLeader = me->SummonCreature(ENTRY_WELL_DOOMGUARD_DEVASTATOR,demonWellSummonPos2,TEMPSUMMON_TIMED_DESPAWN,12000))
                    {
                        for (uint32 i = 0; i < 5; i++)
                        {
                            x = demonWellSummonPos2.GetPositionX();
                            y = demonWellSummonPos2.GetPositionY();
                            z = demonWellSummonPos2.GetPositionZ();
                            o = demonWellSummonPos2.GetOrientation() + (M_PI / 3.0f) * (i + 1);

                            GetLinePoint(x, y, 10.0f, MapManager::NormalizeOrientation(o));

                            me->SummonCreature(ENTRY_WELL_DOOMGUARD_DEVASTATOR, x, y, z, demonWellSummonPos2.GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 12000);
                        }
                    }
                }
                summonDemonsTimer = 10000;
            }
            else summonDemonsTimer -= diff;

            if (eventStarted == false)
                return;

            if (quoteTimer <= diff)
            {
                if (quoteStep < MAX_QUOTES)
                {
                    HandleQuote();
                    quoteStep++;
                    quoteTimer = quoteEvents[quoteStep].nextEventTime;
                }
            }
            else quoteTimer -= diff;

            if (beamsTimer <= diff)
            {
                for (uint32 entry = ENTRY_YSERA_ASPECT; entry <= ENTRY_SORIDORMI_ASPECT; entry++)
                {
                    if (Creature * pAspect = me->FindNearestCreature(entry, me->GetMap()->GetVisibilityDistance(), true))
                    {
                        if (entry == ENTRY_YSERA_ASPECT)
                            pAspect->CastSpell(me, SPELL_GOOD_YSERA_BEAM, false);
                        else if (entry == ENTRY_ALEXSTRASZA_ASPECT)
                            pAspect->CastSpell(me, SPELL_GOOD_ALEX_BEAM, false);
                        else 
                            pAspect->CastSpell(me, SPELL_GOOD_SORI_BEAM, false);
                    }
                }
                beamsTimer = MAX_TIMER;
            }
            else beamsTimer -= diff;

            if (textEmoteTimer <= diff)
            {
                me->MonsterTextEmote("A dark force from within the Dragon Soul reacts violently to the cleansing!", 0, true, me->GetMap()->GetVisibilityDistance());
                textEmoteTimer = MAX_TIMER;
            }
            else textEmoteTimer -= diff;
        }
    };
};

class npc_aspect_woe : public CreatureScript
{
public:
    npc_aspect_woe() : CreatureScript("npc_aspect_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_aspect_woeAI(creature);
    }

    struct npc_aspect_woeAI : public ScriptedAI
    {
        npc_aspect_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_FLYING_ANIM_KIT_AURA, true);
            evilBeamTimer = MAX_TIMER;
            passengers.clear();
            passengerStep = 0;

            if (me->GetEntry() == ENTRY_NELTHARION_ASPECT)
            {
                BoardNeltahrionPassengers();
                evilBeamTimer = 3000;
            }
        }

        uint32 evilBeamTimer;
        std::vector<uint64> passengers;
        uint32 passengerStep;

        void BoardNeltahrionPassengers()
        {
            if (Vehicle * veh = me->GetVehicleKit())
            {
                uint8 seats = veh->m_Seats.size();

                for (uint8 i = 0; i < seats; i++)
                {
                    if (Creature * vehiclePassenger = me->SummonCreature(ENTRY_DEATHWING_VEHICLE_PASSENGER, 0, 0, 0))
                    {
                        passengers.push_back(vehiclePassenger->GetGUID());
                        vehiclePassenger->EnterVehicle(me, i);
                        vehiclePassenger->ClearUnitState(UNIT_STATE_UNATTACKABLE);
                        vehiclePassenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        vehiclePassenger->ForcedDespawn(MINUTE_IN_MILIS);
                    }
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 0)
            if (Creature * pSoul = me->FindNearestCreature(ENTRY_DRAGON_SOUL,250.0f,true))
            if (me->GetEntry() != ENTRY_NELTHARION_ASPECT)
                me->SetFacingTo(me->GetAngle(pSoul));
        }

        void SpellHit(Unit *, const SpellEntry * spell) override
        {
            if (spell->Id == SPELL_DRAGON_SOUL_EXPLOSION)
            {
                me->ForcedDespawn(2000);

                if (Vehicle * veh = me->GetVehicleKit())
                {
                    uint8 seats = veh->m_Seats.size();

                    for (uint8 i = 0; i < seats; i++)
                        if (Creature * passenger = (Creature*)veh->GetPassenger(i))
                        {
                            passenger->ExitVehicle();
                            passenger->ForcedDespawn();
                        }
                }
            }
        }

        void Reset() override
        {
        }

        void UpdateAI(const uint32 diff) override
        {
            if (evilBeamTimer <= diff)
            {
                if (passengerStep < passengers.size())
                {
                    if (Creature * dragonSoul = me->FindNearestCreature(ENTRY_DRAGON_SOUL,250.0f,true))
                    if (Creature * pPassenger = ObjectAccessor::GetCreature(*me, passengers[passengerStep]))
                    {
                        dragonSoul->CastSpell(pPassenger, SPELL_EVIL_DRAGON_SOUL_BEAM, false);
                    }
                    passengers.erase(passengers.begin() + 1);
                    evilBeamTimer = 3000;
                }
                else
                {
                    passengers.clear();
                    evilBeamTimer = MAX_TIMER;
                }
                passengerStep++;
            }
            else evilBeamTimer -= diff;
        }
    };
};

class npc_bronze_drake_vehicle_woe : public CreatureScript
{
public:
    npc_bronze_drake_vehicle_woe() : CreatureScript("npc_bronze_drake_vehicle_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_bronze_drake_vehicle_woeAI(creature);
    }

    struct npc_bronze_drake_vehicle_woeAI : public ScriptedAI
    {
        npc_bronze_drake_vehicle_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            passiveTimer = 5000;
            introQuoteTimer = 3000;
            passengerBoarded = false;
            canInteract = false;
            flightTimer = MAX_TIMER;
            distanceCheckTimer = MAX_TIMER;
            m_number = 0;
        }

        uint32 passiveTimer;
        uint32 introQuoteTimer;
        uint32 flightTimer;
        uint32 distanceCheckTimer;
        uint32 m_number;
        bool passengerBoarded;
        bool canInteract;

        void MoveInLineOfSight(Unit * who) override
        {
            if (!canInteract || passengerBoarded)
                return;

            if (me->IsVehicle() && who->GetTypeId() == TYPEID_PLAYER && who->GetExactDist(me) < 6.0f)
            {
                passengerBoarded = true;
                who->EnterVehicle(me, 0, nullptr);
                flightTimer = 3000;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            if (type == DATA_DRAKE_NUMBER && data < MAX_VEHICLES)
                m_number = data;
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_VEHICLE_MOVE)
            {
                me->GetMotionMaster()->MovePoint(1,vehicleEndPos[m_number].GetPositionX(),
                                                               vehicleEndPos[m_number].GetPositionY(),
                                                               vehicleEndPos[m_number].GetPositionZ(),
                                                               false,true);
                distanceCheckTimer = 1000;
            }
        }

        void Reset() override
        {
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 0)
            {
                if (Creature * pDragonSoul = me->FindNearestCreature(ENTRY_DRAGON_SOUL, me->GetMap()->GetVisibilityDistance(), true))
                    pDragonSoul->AI()->DoAction(ACTION_BRONZE_DRAKE_ARRIVED);
            }
            else if (id == 1)
            {
                if (Vehicle * veh = me->GetVehicleKit())
                {
                    if (Unit * passenger = veh->GetPassenger(0))
                    {
                        passenger->ExitVehicle();
                        passenger->GetMotionMaster()->MoveJump(landingPos.GetPositionX(),
                                                               landingPos.GetPositionY(),
                                                               landingPos.GetPositionZ(),
                                                               20.0f, 10.0f);
                    }
                }
                me->Kill(me); // TODO: Killed by shadowbats ...
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (introQuoteTimer <= diff)
            {
                if (m_number == 1)
                {
                    me->MonsterYell(drakeIntroQuote.text,0,0);
                    me->PlayDirectSound(drakeIntroQuote.soundID);
                }
                introQuoteTimer = MAX_TIMER;
            }
            else introQuoteTimer -= diff;

            if (passiveTimer <= diff)
            {
                canInteract = true;
                passiveTimer = MAX_TIMER;
            }
            else passiveTimer -= diff;

            if (passengerBoarded == false)
                return;

            if (flightTimer <= diff)
            {
                me->GetMotionMaster()->MovePoint(0,
                                            vehicleMiddlePos[m_number].GetPositionX(),
                                            vehicleMiddlePos[m_number].GetPositionY(),
                                            vehicleMiddlePos[m_number].GetPositionZ(), false, true);

                flightTimer = MAX_TIMER;
            }
            else flightTimer -= diff;

            if (distanceCheckTimer <= diff)
            {
                float x = vehicleEndPos[m_number].GetPositionX();
                float y = vehicleEndPos[m_number].GetPositionY();
                float z = vehicleEndPos[m_number].GetPositionZ();
                float o = vehicleEndPos[m_number].GetOrientation();

                if (me->GetExactDist(x,y,z) < 20.0f)
                {
                    GetLinePoint(x, y, 20.0f, o);
                    if (Creature * pShadowBat = me->SummonCreature(ENTRY_SHADOWBAT_DRAKE_KILLER, x,y,z,o, TEMPSUMMON_TIMED_DESPAWN, 5000))
                    {
                        pShadowBat->CastSpell(pShadowBat, 103714, true); // cosmetic shadow form
                        pShadowBat->SetReactState(REACT_PASSIVE);
                        pShadowBat->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        pShadowBat->GetMotionMaster()->MovePoint(0, vehicleEndPos[m_number]);
                    }
                    distanceCheckTimer = MAX_TIMER;
                    return;
                }
                distanceCheckTimer = 500;
            }
            else distanceCheckTimer -= diff;
        }
    };
};

class spell_gen_dragon_soul_explosion : public SpellScriptLoader
{
    public:
        spell_gen_dragon_soul_explosion() : SpellScriptLoader("spell_gen_dragon_soul_explosion") { }

        class spell_gen_dragon_soul_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_dragon_soul_explosion_SpellScript);

            void RemoveNotAspectTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();
                Unit* caster = GetCaster();

                for (uint32 entry = ENTRY_YSERA_ASPECT; entry <= ENTRY_SORIDORMI_ASPECT; entry++)
                {
                    if (Creature * pAspect = caster->FindNearestCreature(entry, caster->GetMap()->GetVisibilityDistance(), true))
                        unitList.push_back(pAspect);
                }

                if (Creature * pAspect = caster->FindNearestCreature(ENTRY_NELTHARION_ASPECT, caster->GetMap()->GetVisibilityDistance(), true))
                    unitList.push_back(pAspect);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_dragon_soul_explosion_SpellScript::RemoveNotAspectTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_dragon_soul_explosion_SpellScript();
        }
};

// BASIC TEMPLATE FOR CREATURE SCRIPTS
/*class Legion_demon_WoE : public CreatureScript
{
public:
    Legion_demon_WoE() : CreatureScript("Legion_demon_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Legion_demon_WoEAI(creature);
    }

    struct Legion_demon_WoEAI : public ScriptedAI
    {
        Legion_demon_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;

        void Reset() override
        {
        }

        void UpdateAI(const uint32 diff) override
        {
        }
    };
};*/


void AddSC_woe_aspects_event()
{
    new npc_the_dragon_soul_woe();
    new npc_aspect_woe();
    new npc_bronze_drake_vehicle_woe();
    new spell_gen_dragon_soul_explosion();
}