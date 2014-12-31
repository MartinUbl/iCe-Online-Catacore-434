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

#define MAX_VEHICLES 5
#define MINUTE_IN_MILIS 60000

const Position dragonSoulPos = { 3396.95f, -5443.50f, 252.14f, 4.80f }; //  Dragon_soul_possition

const Position vehicleStartPos [MAX_VEHICLES] = 
{
    {3471.37f,-5279.54f,229.94f,1.76f},
    {3461.31f,-5287.13f,232.28f,1.52f},
    {3453.41f,-5281.62f,229.94f,1.15f},
    {3442.69f,-5282.94f,231.33f,1.30f},
    {3436.57f,-5276.92f,229.94f,1.21f}
};

const Position vehicleMiddlePos [MAX_VEHICLES] = 
{
    {3425.34f,-5380.07f,248.22f,4.47f},
    {3412.19f,-5381.41f,255.75f,4.60f},
    {3398.63f,-5381.25f,248.22f,4.75f},
    {3389.09f,-5380.96f,257.25f,4.79f},
    {3377.01f,-5380.32f,249.23f,4.72f}
};

const Position vehicleEndePos [MAX_VEHICLES] = 
{
    {3257.50f,-5533.58f,85.80f,3.50f},
    {3255.93f,-5523.60f,94.30f,3.70f},
    {3251.72f,-5517.26f,85.80f,3.54f},
    {3250.82f,-5507.04f,92.50f,3.64f},
    {3244.67f,-5498.17f,85.80f,3.64f}
};

const Position landingPos = { 3132.86f, -5559.66f, 18.05f, 3.68f };

const Position YseraSpawnPos = {3270.65f,-5787.02f,193.14f,1.16f};
const Position YseraEndPos = {3370.05f,-5504.02f,261.17f,1.14f};

const Position AlexstraszaSpawnPos = {3526.48f,-5779.57f,370.49f,1.96f};
const Position AlexstraszaEndPos = {3416.98f,-5513.10f,261.17f,1.90f};

const Position SoridormiSpawnPos = {3060.51f,-5504.43f,270.87f,0.08f};
const Position SoridormiEndPos = {3332.80f,-5456.17f,261.70f,0.51f};

const Position NeltharionSpawnPos = {3565.99f,-5611.27f,337.80f,2.24f};
const Position NeltharionEndPos = {3453.13f,-5497.21f,271.09f,2.35f};

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
    {NEVER, ENTRY_BRONZE_DRAKE_VEHICLE, "The link to the portal must be broken! Quickly, to Stormrage!", 26036}
};

/* “It is them!” 26030
“Heroes! We have been sent by Nozdormu! Quickly, on our backs, we must get you to the Dragon Soul!” 26032 */

enum spells
{
    SPELL_THE_DRAGON_SOUL_COSMETIC = 102845,
};

enum beamSpells
{
    SPELL_EVIL_DRAGON_SOUL_BEAM = 108759,
    SPELL_GOOD_ALEX_BEAM = 107618,
    SPELL_GOOD_YSERA_BEAM = 108615,
    SPELL_GOOD_SORI_BEAM = 108616 
};

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
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_THE_DRAGON_SOUL_COSMETIC, true);
            eventStarted = false;
            quoteStep = 0;
        }

        void SetTimers()
        {
            quoteTimer = 5000;
            beamsTimer = 33000;
            textEmoteTimer = 41000;
        }

        bool eventStarted;
        InstanceScript * pInstance;
        // Generic quote stuff
        uint32 quoteTimer;
        uint32 quoteStep;
        // Other timers
        uint32 beamsTimer;
        uint32 textEmoteTimer;

        void Reset() override {}

        void ReceiveEmote(Player* pPlayer, uint32 text_emote) // This is only temporary
        {
            eventStarted = true;
            SetTimers();
        }

        void HandleQuote()
        {
            // Some special events
            switch (quoteStep)
            {
                case 1:
                {
                    me->SummonCreature(ENTRY_YSERA_ASPECT, YseraEndPos, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS);

                    me->SummonCreature(ENTRY_ALEXSTRASZA_ASPECT, AlexstraszaEndPos, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS);

                    me->SummonCreature(ENTRY_SORIDORMI_ASPECT, SoridormiEndPos, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS);
                    break;
                }
                case 3:
                {
                    for (uint32 entry = ENTRY_YSERA_ASPECT; entry <= ENTRY_SORIDORMI_ASPECT; entry++)
                    {
                        if (Creature * pAspect = me->FindNearestCreature(entry, 250.0f, true))
                            pAspect->InterruptNonMeleeSpells(false);
                    }
                    break;
                }
                case 4:
                {
                    if (Creature * pNeltharion = me->SummonCreature(ENTRY_NELTHARION_ASPECT, NeltharionSpawnPos, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE_IN_MILIS))
                    {
                        pNeltharion->GetMotionMaster()->MovePoint(0,
                                                                  NeltharionEndPos.GetPositionX(),
                                                                  NeltharionEndPos.GetPositionY(),
                                                                  NeltharionEndPos.GetPositionZ(), false, true);
                    }
                      break;
                }
                default:
                    break;
            }

            // Process current quote
            QUOTE_EVENTS qEvent = quoteEvents[quoteStep];

            if (Creature * pCreature = me->FindNearestCreature(qEvent.entry,250.0f,true))
            {
                pCreature->MonsterYell(qEvent.yellQuote, 0, 0);
                pCreature->PlayDistanceSound(qEvent.soundID);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (eventStarted == false)
                return;

            if (quoteTimer <= diff)
            {
                HandleQuote();
                quoteStep++;
                quoteTimer = quoteEvents[quoteStep].nextEventTime;
            }
            else quoteTimer -= diff;

            if (beamsTimer <= diff)
            {
                for (uint32 entry = ENTRY_YSERA_ASPECT; entry <= ENTRY_SORIDORMI_ASPECT; entry++)
                {
                    if (Creature * pAspect = me->FindNearestCreature(entry, 250.0f, true))
                    {
                        if (entry == ENTRY_YSERA_ASPECT)
                            pAspect->CastSpell(me, SPELL_GOOD_YSERA_BEAM, false);
                        else if (entry == ENTRY_ALEXSTRASZA_ASPECT)
                            pAspect->CastSpell(me, SPELL_GOOD_ALEX_BEAM, false);
                        else 
                            pAspect->CastSpell(me, SPELL_GOOD_SORI_BEAM, false);
                    }
                }
                beamsTimer = NEVER;
            }
            else beamsTimer -= diff;

            if (textEmoteTimer <= diff)
            {
                me->MonsterTextEmote("A dark force from within the Dragon Soul reacts violently to the cleansing!", 0, false, 500.0f);
                textEmoteTimer = NEVER;
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
            me->CastSpell(me, 98723, true); // force fly anim
            evilBeamTimer = NEVER;
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
            if (me->IsVehicle() == false)
                return;

            //uint8 seats = vehicle->m_Seats.size();
            uint8 seats = 7;

            for (uint8 i = 0; i < seats; i++)
            {
                if (Creature * vehiclePassenger = me->SummonCreature(ENTRY_DEATHWING_VEHICLE_PASSENGER, 0, 0, 0))
                {
                    passengers.push_back(vehiclePassenger->GetGUID());
                    vehiclePassenger->EnterVehicle(me, i);
                    vehiclePassenger->ClearUnitState(UNIT_STATE_UNATTACKABLE);
                    vehiclePassenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    vehiclePassenger->ForcedDespawn(2 * MINUTE_IN_MILIS);
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
                    evilBeamTimer = NEVER;
                }
                passengerStep++;
            }
            else evilBeamTimer -= diff;
        }
    };
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
}