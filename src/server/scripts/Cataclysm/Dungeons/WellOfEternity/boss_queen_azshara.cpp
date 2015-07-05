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

enum spells
{
    SPELL_SHROUD_OF_LUMINOSITY = 102915, // immune to damage
    SPELL_SERVANT_OF_THE_QUEEN = 102334,
    SPELL_TOTAL_OBEDIENCE_CHARM = 103241, // + change faction + vehicle kit
    SPELL_TOTAL_OBEDIENCE_STUN = 110096, // Target Aura Spell (102334) Servant of the Queen -> (can't do that yet) !!!
    SPELL_PUPPET_STRING_HOVER = 102312,
    SPELL_PUPPET_CROSS_VISUAL = 102310,
    SPELL_SHADOWBAT_COSMETIC = 103714,
    SPELL_STAND_STATE_COSMETIC = 93324 // force stand state during combat (workaround)
};

enum channelSpells
{
    SPELL_FROST_CHANNELING = 110492,
    SPELL_FIRE_CHANNELING = 110494,
    SPELL_ARCANE_CHANNELING = 110495
};

enum entries
{
    ENTRY_QUEEN_AZSHARA = 54853,

    ENTRY_FROST_MAGUS = 54883,
    ENTRY_FIRE_MAGUS = 54882,
    ENTRY_ARCANE_MAGUS = 54884,

    ENTRY_ROYAL_HANDMAIDEN = 54645,
    ENTRY_SHADOWBAT_VEHICLE = 57117,
    ENTRY_CAPTAIN_VAROTHEN = 57118,

    ENTRY_HAND_OF_THE_QUEEN = 54728,
    ENTRY_ARCANE_CIRCLE = 54639, // not sure if good id :P
};

#define MAX_MAGES 6

static const Position magesPos[MAX_MAGES] =
{
    {3470.93f,-5281.00f, 229.94f, 4.96f},
    {3461.87f,-5282.56f, 229.94f, 4.71f},
    {3452.88f,-5281.91f, 229.94f, 4.50f},
    {3444.06f,-5279.93f, 229.94f, 4.36f},
    {3434.84f,-5277.00f, 229.94f, 4.13f},
    {3427.60f,-5272.47f, 229.94f, 3.94f}
};

static void PlayQuote (Creature * source, SimpleQuote quote, bool yell = true)
{
    source->PlayDirectSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}

static const SimpleQuote aggroQuote = { 26013, "Ah, welcome. You are here to join us in the coming celebration? No? A pity." };

static const SimpleQuote firstMagus[2] =
{
    { 26027, "I have no time for such diversions. Keepers of Eternity, will you stand for your queen?" },
    { 26045, "I pray that the Light of a Thousand Moons will grant me this honor." }
};

static const SimpleQuote secondMagus[2] =
{
    { 26028, "Still these strangers would oppose your queen's will. Who will stop them?" },
    { 26046, "Yes, Light of Lights! My life is yours!" }
};

static const SimpleQuote thirdMagus[2] =
{
    { 26029, "I beseech of you, my beloved subjects: Put an end to these miscreants." },
    { 26047, "The Flower of Life calls upon me. I WILL NOT fail you, my Queen." }
};

static const SimpleQuote puppetTurn[2] =
{
    { 26025, "If you intend to play the fool, you may as well look the part." },
    { 26024, "Dance for the Eternal Court." }
};
static const SimpleQuote puppetDanceQuote = { 26026, "Serve Azshara, puppets, and rejoice." };

static const SimpleQuote interruptQuotes[3] =
{
    { 26014, "Bold of you, to strike a queen. A lesser monarch might be enraged." },
    { 26016, "Do not ask for mercy after such an act." },
    { 26015, "Such insolence! My temper grows short." }
};

static const SimpleQuote killQuotes[3] =
{
    { 26023, "Unfortunate, but deserved." },
    { 26021, "I am unimpressed." },
    { 26022, "Your conduct was inexcusable." }
};

static const SimpleQuote wipeQuote = { 26020, "To prepare for a world of perfection, the imperfect must be swept away." };

static const SimpleQuote leavingQuotes[4] =
{
    { 26017, "Enough! As much as I adore playing hostess, I have more pressing matters to attend to." },
    { 26018, "Riders, to me!" },
    { 26136, "At your side, my queen!" }, // shadowbat quote
    { 26019, "My noble Varo'then, do return and dispose of this murderous band." } // say
};

enum phases
{
    PHASE_COMBAT = 0,
    PHASE_LEAVING
};

enum magusWave
{
    MAGUS_WAVE_FIRST = 1,
    MAGUS_WAVE_SECOND = 2,
    MAGUS_WAVE_THIRD = 3,
    MAX_MAGUS_WAVES = 4
};

enum actions
{
    ACTION_CALL_MAGUS,
    ACTION_RELEASE_MAGUS,
    ACTION_MAGUS_DIED,
};

#define GO_ROYAL_CHEST 210025

static uint32 magusEntries[MAX_MAGES] = {ENTRY_FROST_MAGUS,ENTRY_ARCANE_MAGUS,ENTRY_FIRE_MAGUS,ENTRY_FROST_MAGUS,ENTRY_FIRE_MAGUS,ENTRY_ARCANE_MAGUS};

class boss_queen_azshara_woe : public CreatureScript
{
public:
    boss_queen_azshara_woe() : CreatureScript("boss_queen_azshara_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_queen_azshara_woeAI(creature);
    }

    struct boss_queen_azshara_woeAI : public ScriptedAI
    {
        boss_queen_azshara_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            encounterComplete = false;
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
            pInstance = me->GetInstanceScript();
            magesGUIDs.clear();
            me->CastSpell(me, SPELL_SHROUD_OF_LUMINOSITY, true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            for (int i = 0; i < MAX_MAGES; i++)
                if (Creature * magus = me->SummonCreature(magusEntries[i], magesPos[i].GetPositionX(), magesPos[i].GetPositionY(), magesPos[i].GetPositionZ(), magesPos[i].GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
                    magesGUIDs.push_back(magus->GetGUID());
                else
                    magesGUIDs.push_back(0);
        }

        void RespawnMages()
        {
            for (uint32 i = 0; i < magesGUIDs.size(); i++)
                if (Creature * magus = ObjectAccessor::GetCreature(*me,magesGUIDs[i]))
                {
                    magus->CombatStop(true);
                    magus->AI()->EnterEvadeMode();
                    magus->setDeathState(JUST_ALIVED);
                    magus->GetMotionMaster()->MoveTargetedHome();
                }
        }

        std::vector<uint64> magesGUIDs;
        InstanceScript * pInstance;
        bool encounterComplete;
        bool totalObedienceInterrupted;
        uint32 interruptCounter;

        // Magus stuff
        uint32 magusWave;
        uint32 magusReleaseTimer;
        uint32 magusWaveTimer;
        uint32 magusPeriodicSummonTimer;
        uint32 magusRemaining;
        uint32 magesToCall;
        // Combat Timers
        uint32 servantTimer;
        uint32 obedienceTimer;
        uint32 obedienceCheckTimer;
        // After combat timers
        uint32 dialogTimer;
        // Phasing
        uint32 PHASE;

        void Reset() override
        {
            totalObedienceInterrupted = false;
            interruptCounter = 0;
            magusWave = 0;
            magusReleaseTimer = MAX_TIMER;
            obedienceCheckTimer = MAX_TIMER;
            magusWaveTimer = 12000;
            magusPeriodicSummonTimer = magusWaveTimer + 60000;
            magusRemaining = MAX_MAGES;
            magesToCall = 1;
            servantTimer = 26000;
            obedienceTimer = 45000;
            PHASE = PHASE_COMBAT;

            if (pInstance)
            {
                pInstance->SetData(DATA_QUEEN_AZSHARA, NOT_STARTED);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
            me->CastSpell(me, SPELL_STAND_STATE_COSMETIC, true);

            if (!me->HasAura(SPELL_SHROUD_OF_LUMINOSITY))
                me->CastSpell(me, SPELL_SHROUD_OF_LUMINOSITY, true);
        }

        void EnterCombat(Unit * who) override
        {
            me->SetInCombatWithZone();
            me->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            PlayQuote(me, aggroQuote);
            magusWaveTimer = 12000;

            if (pInstance)
            {
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                pInstance->SetData(DATA_QUEEN_AZSHARA, IN_PROGRESS);
            }
            ScriptedAI::EnterCombat(who);
        }

        void EnterEvadeMode() override
        {
            PlayQuote(me, wipeQuote);
            RespawnMages();

            if (pInstance)
            {
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                pInstance->SetData(DATA_QUEEN_AZSHARA, NOT_STARTED);
            }

            Map::PlayerList const& lPlayers = pInstance->instance->GetPlayers();

            if (!lPlayers.isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                    if (Player *pPlayer = itr->getSource())
                    {
                        if (pPlayer->HasAura(SPELL_SERVANT_OF_THE_QUEEN))
                        {
                            if (Vehicle * veh = pPlayer->GetVehicleKit())
                                if (Unit * passenger = veh->GetPassenger(0))
                                    if (passenger->GetTypeId() == TYPEID_UNIT)
                                    {
                                        passenger->ExitVehicle();
                                        passenger->Kill(passenger);
                                        passenger->ToCreature()->ForcedDespawn(2000);
                                    }

                            pPlayer->RemoveAura(SPELL_SERVANT_OF_THE_QUEEN);
                            me->Kill(pPlayer);
                        }
                    }
            }
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit * victim)
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                PlayQuote(me, killQuotes[urand(0,2)]);
            }
        }

        void SummonMaidens()
        {
            if (Creature * pMaiden = me->SummonCreature(ENTRY_ROYAL_HANDMAIDEN, 3448.7f, -5226.0f, 230.6f, 5.32f, TEMPSUMMON_TIMED_DESPAWN, 5 * 60000))
            {
                pMaiden->setFaction(35);
                pMaiden->GetMotionMaster()->MovePoint(0, 3458.0f, -5245.8f, 230.0f, false, true);
                pMaiden->SetWalk(true);
                pMaiden->SetSpeed(MOVE_WALK, 1.0f, true);
            }
            if (Creature * pMaiden = me->SummonCreature(ENTRY_ROYAL_HANDMAIDEN, 3469.47f,-5225.0f,230.6f,4.8f, TEMPSUMMON_TIMED_DESPAWN, 5 * 60000))
            {
                pMaiden->setFaction(35);
                pMaiden->GetMotionMaster()->MovePoint(0, 3470.0f, -5247.6f, 230.0f, false, true);
                pMaiden->SetWalk(true);
                pMaiden->SetSpeed(MOVE_WALK, 1.0f, true);
            }
        }

        void SpellCastInterrupted(const SpellEntry* spell) override
        {
            if (spell->Id != SPELL_TOTAL_OBEDIENCE_CHARM)
                return;

            totalObedienceInterrupted = true;

            if (interruptCounter < 3)
                PlayQuote(me, interruptQuotes[interruptCounter++]);
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_MAGUS_DIED)
            {
                magesToCall++;
                magusWaveTimer = 1;

                magusRemaining--;

                if (magusRemaining == 0)
                {
                    encounterComplete = true;
                    // Summon chest
                    if (Player * pPlayer = SelectRandomPlayer(250.0f))
                        pPlayer->SummonGameObject(GO_ROYAL_CHEST, 3464.8f, -5244.4f, 230.0f, 4.55f, 0, 0, 0, 0, 0);

                    if (pInstance)
                    {
                        pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        pInstance->SetData(DATA_QUEEN_AZSHARA, DONE);
                    }

                    if (Map * map = me->GetMap())
                    {
                        for (Map::PlayerList::const_iterator i = map->GetPlayers().begin(); i != map->GetPlayers().end(); ++i)
                        {
                            Player * player = i->getSource();
                            if (player && player->GetQuestStatus(QUEST_THE_VAINGLORIOUS) == QUEST_STATUS_INCOMPLETE)
                            {
                                player->KilledMonsterCredit(me->GetEntry(), 0);
                            }
                        }
                    }

                    // Summon friendly maidens
                    SummonMaidens();
                    // Cancel combat
                    me->RemoveAllAuras();
                    me->DeleteThreatList();
                    me->CombatStop(true);
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    PlayQuote(me, leavingQuotes[0]);
                    // Call taxi
                    me->SummonCreature(ENTRY_SHADOWBAT_VEHICLE, 3439.0f, -5201.0f, 275.0f, 5.0f);
                    // Prepare for post encounter dialog
                    dialogTimer = 7000;
                }
            }
        }

        void PlayMagusWaveQuote(uint32 magusWave)
        {
            switch (magusWave)
            {
                case MAGUS_WAVE_FIRST:
                    PlayQuote(me, firstMagus[0]);
                    break;
                case MAGUS_WAVE_SECOND:
                    PlayQuote(me, secondMagus[0]);
                    break;
                case MAGUS_WAVE_THIRD:
                    PlayQuote(me, thirdMagus[0]);
                    break;
            }
        }

        void ReleaseMages(uint32 _magusWave)
        {
            uint32 index1,index2;

            switch (_magusWave)
            {
                case MAGUS_WAVE_FIRST:
                    index1 = 2;
                    index2 = 3;
                    break;
                case MAGUS_WAVE_SECOND:
                    index1 = 1;
                    index2 = 4;
                    break;
                case MAGUS_WAVE_THIRD:
                    index1 = 0;
                    index2 = 5;
                    break;
                default: // Should never happen
                    index1 = 0;
                    index2 = 1;
                    break;
            }

            Creature * pMagus1 = ObjectAccessor::GetCreature(*me, magesGUIDs[index1]);
            Creature * pMagus2 = ObjectAccessor::GetCreature(*me, magesGUIDs[index2]);

            if (pMagus1 && pMagus2)
            {
                pMagus1->AI()->DoAction(ACTION_CALL_MAGUS);
                pMagus2->AI()->DoAction(ACTION_CALL_MAGUS);

                switch (magusWave)
                {
                    case MAGUS_WAVE_FIRST:
                        PlayQuote(pMagus1, firstMagus[1]);
                        break;
                    case MAGUS_WAVE_SECOND:
                        PlayQuote(pMagus1, secondMagus[1]);
                        break;
                    case MAGUS_WAVE_THIRD:
                        PlayQuote(pMagus1, thirdMagus[1]);
                        break;
                }
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 0)
                PlayQuote(me, leavingQuotes[1]);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (encounterComplete == true)
            {
                if (dialogTimer <= diff)
                {
                    me->RemoveAllAuras(); // Remove shroud
                    me->GetMotionMaster()->MovePoint(0, 3455.8f,-5249.5f,230.4f, false, true);
                    dialogTimer = MAX_TIMER;
                }
                else dialogTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (!me->HasAura(SPELL_STAND_STATE_COSMETIC) && !me->IsNonMeleeSpellCasted(false))
                me->CastSpell(me, SPELL_STAND_STATE_COSMETIC, true);

            if (magusWaveTimer <= diff)
            {
                if (++magusWave >= MAX_MAGUS_WAVES)
                {
                    magusWaveTimer = MAX_TIMER;
                    return;
                }

                magusPeriodicSummonTimer = 60000;

                if (magesToCall == 1)
                    PlayMagusWaveQuote(magusWave);

                magusReleaseTimer = 5000;
                magusWaveTimer = MAX_TIMER;
            }
            else magusWaveTimer -= diff;

            if (magusReleaseTimer <= diff)
            {
                uint32 _magesToSummon = magesToCall;
                for (uint32 i = 0; i < _magesToSummon; i++)
                {
                    ReleaseMages(magusWave - i);
                    magesToCall--;
                }
                magusReleaseTimer = MAX_TIMER;
            }
            else magusReleaseTimer -= diff;

            if (magusPeriodicSummonTimer <= diff)
            {
                if (++magusWave >= MAX_MAGUS_WAVES)
                {
                    magusPeriodicSummonTimer = MAX_TIMER;
                    return;
                }
                ReleaseMages(magusWave);
                magusPeriodicSummonTimer = 60000;
            }
            else magusPeriodicSummonTimer -= diff;

            // COMBAT TIMERS
            if (servantTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    // If we found player without SPELL_SERVANT_OF_THE_QUEEN aura -> cast it
                    Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true, -SPELL_SERVANT_OF_THE_QUEEN);

                    uint32 combatPlayers = me->getThreatManager().getThreatList().size();

                    if (player != nullptr && combatPlayers > 1)
                    {
                        me->AddAura(SPELL_SERVANT_OF_THE_QUEEN, player);

                        if (Vehicle * veh = player->GetVehicleKit())
                        {
                            if (Creature * pHand = me->SummonCreature(ENTRY_HAND_OF_THE_QUEEN, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), 0.0f))
                            {
                                pHand->setFaction(me->getFaction());
                                pHand->EnterVehicle(veh, 0);
                                pHand->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            }
                        }
                    }
                    else
                    {
                        // If we did'nt find any valid player -> remove vehicle kit and kill players
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SERVANT_OF_THE_QUEEN);

                        Map::PlayerList const& lPlayers = pInstance->instance->GetPlayers();

                        if (!lPlayers.isEmpty() && combatPlayers > 1)
                        {
                            for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                                if (Player *pPlayer = itr->getSource())
                                {
                                    if (pPlayer->GetDistance(me) <= 100.0f && !pPlayer->IsGameMaster())
                                        me->Kill(pPlayer);
                                }
                        }
                    }
                    servantTimer = 58000;
                }
            }
            else servantTimer -= diff;

            if (obedienceTimer <= diff)
            {
                me->MonsterTextEmote("Queen Azshara begins to transform everybody into puppets! You must interrupt her!", 0, true);
                PlayQuote(me, puppetTurn[urand(0,1)]);
                me->RemoveAura(SPELL_STAND_STATE_COSMETIC);
                me->CastSpell(me, SPELL_TOTAL_OBEDIENCE_CHARM, false); // 10 second cast time
                totalObedienceInterrupted = false;
                obedienceCheckTimer = 10000;
                obedienceTimer = 73000;
            }
            else obedienceTimer -= diff;

            if (obedienceCheckTimer <= diff)
            {
                if (totalObedienceInterrupted == false)
                {
                    PlayQuote(me, puppetDanceQuote);

                    // TODO: This is workaround, maybe find better solution in future
                    Map::PlayerList const& lPlayers = pInstance->instance->GetPlayers();

                    if (!lPlayers.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                            if (Player *pPlayer = itr->getSource())
                            {
                                if (!pPlayer->IsGameMaster())
                                    me->Kill(pPlayer);
                            }
                    }
                }

                obedienceCheckTimer = MAX_TIMER;
            }
            else obedienceCheckTimer -= diff;
        }
    };
};

enum magusSpells
{
    /*FROST MAGUS*/
    SPELL_ICE_FLING = 102478, // set MaxAffectedTargets to 1 !
    SPELL_COLDFLAME_DUMMY_CAST = 102465, // add periodic dummy aura after cast
    SPELL_COLDFLAME_PERSISTENT_AURA_DMG = 102466, // aoe persistence damage 4s (x,y,z)
    SPELL_BLADES_OF_ICE_CHARGE = 102467,
    SPELL_BLADES_OF_ICE_MISSILE = 102468, // //TODO: Correctly fix weapon damage

    /*FIRE MAGUS*/
    SPELL_FIREBALL = 102265,
    SPELL_BLAST_WAVE = 102483,
    SPELL_FIREBOMB = 102482, // to target

    /*ARCANE MAGUS*/
    SPELL_ARCANE_SHOCK = 102463, // trigerring periodically aoe 102464 
    SPELL_ARCANE_BOMB_DAMAGE = 102455,
    SPELL_ARCANE_BOMB_GROUND_VISUAL = 102460,
    SPELL_ARCANE_BOMB_FALLING_VISUAL = 109122
};

class npc_enchanted_magus_woe : public CreatureScript
{
public:
    npc_enchanted_magus_woe() : CreatureScript("npc_enchanted_magus_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_enchanted_magus_woeAI(creature);
    }

    struct npc_enchanted_magus_woeAI : public ScriptedAI
    {
        npc_enchanted_magus_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            entry = me->GetEntry();
            pInstance = me->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, GetChannelSpellIdByEntry(me->GetEntry()), false);
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
        }

        uint32 GetChannelSpellIdByEntry(uint32 entry)
        {
            switch (entry)
            {
                case ENTRY_ARCANE_MAGUS:
                    return SPELL_ARCANE_CHANNELING;
                case ENTRY_FIRE_MAGUS:
                    return SPELL_FIRE_CHANNELING;
                case ENTRY_FROST_MAGUS:
                    return SPELL_FROST_CHANNELING;
            }
            return 0;
        }

        InstanceScript * pInstance;
        uint32 entry;

        // FIRE TIMERS
        uint32 fireballTimer;
        uint32 blastWaveTimer;
        uint32 fireBombTimer;
        //FROST TIMERS
        uint32 iceFlingTimer;
        uint32 coldFlameTimer;
        uint32 bladeChargeTimer;
        float flameAngle;
        float x, y;
        // ARCANE TIMERS
        uint32 arcaneShockTimer;
        uint32 arcaneBombTimer;

        void Reset() override
        {
            //Fire
            fireballTimer = 100;
            blastWaveTimer = 8000;
            fireBombTimer = 12000;
            //Frost
            iceFlingTimer = 4000;
            coldFlameTimer = 9000;
            bladeChargeTimer = 7000;
            flameAngle = 0.0f;
            x = me->GetPositionX();
            y = me->GetPositionY();
            //Arcane
            arcaneShockTimer = 8000;
            arcaneBombTimer = 2000;
        }

        void EnterEvadeMode() override
        {
            ScriptedAI::EnterEvadeMode();
            me->RemoveAllAuras();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        void JustReachedHome() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, GetChannelSpellIdByEntry(me->GetEntry()), false);
        }

        void KilledUnit(Unit * victim)
        {
            if (Creature * pQueen = me->FindNearestCreature(ENTRY_QUEEN_AZSHARA, 250.0f, true))
                pQueen->AI()->KilledUnit(victim);
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_CALL_MAGUS)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->InterruptNonMeleeSpells(false);
                me->SetInCombatWithZone();
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM,0,200.0f,true))
                {
                    me->AddThreat(player, 0.1f);
                    me->AI()->AttackStart(player);
                }
            }
        }

        void JustDied(Unit *) override
        {
            if (Creature * pQueen = me->FindNearestCreature(ENTRY_QUEEN_AZSHARA, 250.0f, true))
                pQueen->AI()->DoAction(ACTION_MAGUS_DIED);
        }

        //void MoveInLineOfSight(Unit * who) override {} 

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (!UpdateVictim())
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            {
                DoMeleeAttackIfReady();
                return;
            }

            switch (entry)
            {
                case ENTRY_FIRE_MAGUS:
                {
                    if (fireballTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 38.0f, true))
                                me->CastSpell(target, SPELL_FIREBALL, false);
                            fireballTimer = 2100;
                        }
                    }
                    else fireballTimer -= diff;

                    if (blastWaveTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            me->CastSpell(me, SPELL_BLAST_WAVE, false);
                            fireballTimer = 500;
                            blastWaveTimer = urand(10000, 15000);
                        }
                    }
                    else blastWaveTimer -= diff;

                    if (fireBombTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 42.0f, true))
                                me->CastSpell(target, SPELL_FIREBOMB, false);
                            fireballTimer = 500;
                            fireBombTimer = urand(12000, 16000);
                        }
                    }
                    else fireBombTimer -= diff;
                }
                break;
                case ENTRY_FROST_MAGUS:
                {
                    if (iceFlingTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            me->CastSpell(me, SPELL_ICE_FLING, false);
                            iceFlingTimer = urand(11000, 13000);
                        }
                    }
                    else iceFlingTimer -= diff;

                    if (coldFlameTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 28.0f, true))
                            {
                                // Remember these things, they are needed in spellscript
                                flameAngle = me->GetAngle(target);
                                me->GetPosition(x, y);
                                me->CastSpell(target, SPELL_COLDFLAME_DUMMY_CAST, false); // everything else should handle spellscript
                            }
                            coldFlameTimer = urand(23000, 25000);
                        }
                    }
                    else coldFlameTimer -= diff;

                    if (bladeChargeTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 38.0f, true))
                                me->CastSpell(target, SPELL_BLADES_OF_ICE_CHARGE, false);
                            bladeChargeTimer = 10000;
                        }
                    }
                    else bladeChargeTimer -= diff;
                }
                break;
                case ENTRY_ARCANE_MAGUS:
                {
                    if (arcaneShockTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            me->CastSpell(me, SPELL_ARCANE_SHOCK, false);
                            arcaneShockTimer = 20000;
                        }
                    }
                    else arcaneShockTimer -= diff;

                    if (arcaneBombTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                me->SummonCreature(ENTRY_ARCANE_CIRCLE, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(),target->GetOrientation());
                            arcaneBombTimer = 10000;
                        }
                    }
                    else arcaneBombTimer -= diff;
                }
                break;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_arcane_bomb_woe : public CreatureScript
{
public:
    npc_arcane_bomb_woe() : CreatureScript("npc_arcane_bomb_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_arcane_bomb_woeAI(creature);
    }

    struct npc_arcane_bomb_woeAI : public ScriptedAI
    {
        npc_arcane_bomb_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
            me->CastSpell(me, SPELL_ARCANE_BOMB_GROUND_VISUAL, true);
            me->CastSpell(me, SPELL_ARCANE_BOMB_FALLING_VISUAL, true);
            me->ForcedDespawn(10000);
            explodeTimer = 5000;
        }

        uint32 explodeTimer;

        void Reset() override { }
        void MovementInform(uint32 type, uint32 id) override {}

        void EnterEvadeMode() override {}
        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*who*/) { }
        void MoveInLineOfSight(Unit* /*who*/) { }

        void UpdateAI(const uint32 diff) override
        {
            if (explodeTimer <= diff)
            {
                me->RemoveAura(SPELL_ARCANE_BOMB_GROUND_VISUAL);
                me->RemoveAura(SPELL_ARCANE_BOMB_FALLING_VISUAL);
                me->CastSpell(me, SPELL_ARCANE_BOMB_DAMAGE, true);
                me->ForcedDespawn(2000);
                explodeTimer = MAX_TIMER;
            }
            else explodeTimer -= diff;
        }
    };
};


class npc_hand_of_the_queen_woe : public CreatureScript
{
public:
    npc_hand_of_the_queen_woe() : CreatureScript("npc_hand_of_the_queen_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hand_of_the_queen_woeAI(creature);
    }

    struct npc_hand_of_the_queen_woeAI : public ScriptedAI
    {
        npc_hand_of_the_queen_woeAI(Creature* creature) : ScriptedAI(creature) {}

        void JustDied(Unit *) override
        {
            if (Unit * vehBase = me->GetVehicleBase())
            {
                vehBase->RemoveAurasByType(SPELL_AURA_AOE_CHARM); // Drop Servant of the Queen aura from player
            }
            me->ForcedDespawn(10000);
        }
    };
};


enum shadowbatWP
{
    WP_GROUND,
    WP_FLY
};

class npc_queen_shadowbat_woe : public CreatureScript
{
public:
    npc_queen_shadowbat_woe() : CreatureScript("npc_queen_shadowbat_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_queen_shadowbat_woeAI(creature);
    }

    struct npc_queen_shadowbat_woeAI : public ScriptedAI
    {
        npc_queen_shadowbat_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->CastSpell(me, SPELL_SHADOWBAT_COSMETIC, true);
            me->SetFlying(true);

            // Summon and carry captain Varothen
            if (Creature * pCaptain = me->SummonCreature(ENTRY_CAPTAIN_VAROTHEN, 0, 0, 0))
            {
                pCaptain->EnterVehicle(me, 0);

                pCaptain->SetReactState(REACT_PASSIVE);
                pCaptain->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                pCaptain->ForcedDespawn(60000);
            }
            me->GetMotionMaster()->MovePoint(WP_GROUND,3451.0f,-5264.0f,231.0f,false,true);
            me->ForcedDespawn(60000);
            moveTimer = MAX_TIMER;
            moveStep = 0;
        }

        InstanceScript * pInstance;
        uint32 moveTimer;
        uint32 moveStep;

        void Reset() override {}
        void DoAction(const int32 action) override {}

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == WP_GROUND)
            {
                PlayQuote(me, leavingQuotes[2]);
                moveTimer = 4000;
            }
            else if (id == WP_FLY)
            {
                // Remove passengers and despawn them
                if (Creature * pCapitan = me->FindNearestCreature(ENTRY_CAPTAIN_VAROTHEN,20.0,true))
                {
                    pCapitan->ExitVehicle();
                    pCapitan->ForcedDespawn();
                }
                if (Creature * pQueen = me->FindNearestCreature(ENTRY_QUEEN_AZSHARA,20.0,true))
                {
                    pQueen->ExitVehicle();
                    pQueen->setFaction(35);
                    pQueen->SetVisible(false);
                    pQueen->Kill(pQueen);
                    //TODO: Set instance data
                }

                #define ENTRY_DRAGON_SOUL 55078
                if (Creature * pDragonSoul = me->FindNearestCreature(ENTRY_DRAGON_SOUL, 500.0f, true))
                    pDragonSoul->AI()->DoAction(ACTION_SPAWN_DRAKE_VEHICLES);
                // Despaw us also
                me->ForcedDespawn();
            }
        }

        void EnterEvadeMode() override {}
        void AttackStart(Unit*) {}
        void MoveInLineOfSight(Unit * who) override {}

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (moveTimer <= diff)
            {
                switch (moveStep)
                {
                    case 0:
                    {
                        if (Creature * pQueen = me->FindNearestCreature(ENTRY_QUEEN_AZSHARA,250.0f,true))
                            pQueen->EnterVehicle(me, -1);
                        moveTimer = 4000;
                        break;
                    }
                    case 1:
                    {
                        PlayQuote(me, leavingQuotes[3], false);
                        moveTimer = 1000;
                        break;
                    }
                    case 2:
                    {
                        me->GetMotionMaster()->MovePoint(WP_FLY, 3566.33f, -5331.0f, 270.0f, false, true);
                        moveTimer = MAX_TIMER;
                        break;
                    }
                }

                moveStep++;
            }
            else moveTimer -= diff;
        }
    };
};

enum royalMaidenSPells
{
    SPELL_GOLDEN_BOWL = 102415,
    SPELL_PIERCING_THORNS = 102238,
    SPELL_CHOKING_PARFUME = 102208,
    SPELL_SWEET_LULLABY_SLEEP = 102245,
};

class npc_royal_handmaiden_woe : public CreatureScript
{
public:
    npc_royal_handmaiden_woe() : CreatureScript("npc_royal_handmaiden_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_royal_handmaiden_woeAI(creature);
    }

    struct npc_royal_handmaiden_woeAI : public ScriptedAI
    {
        npc_royal_handmaiden_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            isTempSummon = me->ToTempSummon();

            if (isTempSummon)
            {
                me->LoadEquipment(0, true);
                me->CastSpell(me, SPELL_GOLDEN_BOWL, true);
            }
        }

        bool isTempSummon;
        uint32 lullabyTimer;
        uint32 parfumeTimer;

        void Reset() override
        {
            if (!isTempSummon)
                me->CastSpell(me, SPELL_PIERCING_THORNS, true);

            lullabyTimer = urand(5000, 10000);
            parfumeTimer = urand(1000, 7000);
        }

        void JustDied(Unit*) override
        {
            if (Map * map = me->GetMap())
            {
                for (Map::PlayerList::const_iterator i = map->GetPlayers().begin(); i != map->GetPlayers().end(); ++i)
                {
                    Player * player = i->getSource();
                    if (player && player->GetQuestStatus(QUEST_DOCUMENTING_THE_TIMEWAYS) == QUEST_STATUS_INCOMPLETE)
                    {
                        player->CastSpell(me, SPELL_ARCHIVAL_HANDMAIDEN_CHANNEL, true);
                        me->CastSpell(player, SPELL_ARCHIVAL_HANDMAIDEN_CREDIT, true);
                    }
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == 0)
            {
                me->SetFacingTo(4.45f);
                me->SetStandState(UNIT_STAND_STATE_SIT); // Sit in case we are friendly maidens summoned by Queen Azshara
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (lullabyTimer <= diff)
            {
                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                    me->CastSpell(target, SPELL_SWEET_LULLABY_SLEEP, true);

                lullabyTimer = urand(20000,30000);
            }
            else lullabyTimer -= diff;

            if (parfumeTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 20.0f, true))
                        me->CastSpell(target, SPELL_CHOKING_PARFUME, false);
                    parfumeTimer = urand(6000, 8000);
                }
            }
            else parfumeTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class spell_gen_coldflame_woe : public SpellScriptLoader
{
    public:
        spell_gen_coldflame_woe() : SpellScriptLoader("spell_gen_coldflame_woe") { }

        class spell_gen_coldflame_woe_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_coldflame_woe_AuraScript);

            float radius;

            void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
            {
                Unit * magus = GetTarget();
                if (!magus)
                    return;

                if (npc_enchanted_magus_woe::npc_enchanted_magus_woeAI* pAI = (npc_enchanted_magus_woe::npc_enchanted_magus_woeAI*)(magus->GetAI()))
                {
                    radius += 4.0f;
                    float angle = pAI->flameAngle;
                    float x = pAI->x + cos(angle)*radius;
                    float y = pAI->y + sin(angle)*radius;
                    float z = 230.0f;

                    magus->CastSpell(x, y, z, SPELL_COLDFLAME_PERSISTENT_AURA_DMG, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_coldflame_woe_AuraScript::HandlePeriodicTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }

        public:
            spell_gen_coldflame_woe_AuraScript() : AuraScript()
            {
                radius = 0.0f;
            };
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_gen_coldflame_woe_AuraScript();
        }
};

class spell_gen_servant_of_the_queen : public SpellScriptLoader
{
public:
    spell_gen_servant_of_the_queen() : SpellScriptLoader("spell_gen_servant_of_the_queen") { }

    class spell_gen_servant_of_the_queen_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_servant_of_the_queen_AuraScript);

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* player = GetOwner()->ToPlayer();
            if (!player)
                return;

            player->RemoveAura(SPELL_PUPPET_STRING_HOVER);
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* player = GetTarget();
            if (!player)
                return;

            player->AddAura(SPELL_PUPPET_STRING_HOVER, player);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_gen_servant_of_the_queen_AuraScript::OnApply, EFFECT_0, SPELL_AURA_SET_VEHICLE_ID, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_servant_of_the_queen_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SET_VEHICLE_ID, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gen_servant_of_the_queen_AuraScript();
    }
};

void AddSC_boss_queen_azshara()
{
    new boss_queen_azshara_woe(); // 54853
    new npc_enchanted_magus_woe(); // 54882 - 54884 select * from creature_template where entry between 54882 and 54884;
    new npc_queen_shadowbat_woe(); // 57117
    new npc_royal_handmaiden_woe(); // 54645
    new npc_arcane_bomb_woe(); // 54639
    new npc_hand_of_the_queen_woe(); // 54728

    new spell_gen_servant_of_the_queen(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (102334, 'spell_gen_servant_of_the_queen');
    new spell_gen_coldflame_woe(); // INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (102465, 'spell_gen_coldflame_woe');
}