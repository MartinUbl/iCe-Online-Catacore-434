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

#include "well_of_eternity.h"
#include "ScriptPCH.h"
#include "MapManager.h"

#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

#define FIREWALL_GO_ENTRY (207679)

// Middle of the room
#define MIDDLE_X (3334.7f)
#define MIDDLE_Y (-4890.0f)
#define MIDDLE_Z (181.08f)

static void PlayQuote (Creature * source, SimpleQuote quote, bool yell = false)
{
    source->PlayDistanceSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}

static const SimpleQuote introQuotes[3] =
{
    { 26118, "He is near, lurking in the shadows... I can sense it." },
    { 26120, "You, Felguard. Hold this area." },
    { 26121, "The rest of you, secure the courtyard." },
};

static const SimpleQuote eventStart = { 26128, "None will reach the palace without besting Peroth'arn...first of the feltouched" };

static const SimpleQuote aggroQuotes[2] =
{
    { 26112, "No mortal may stand before me and live!" },
    { 26049, "Nothing will stop me! Not even you, Demon!" },
};

static const SimpleQuote essenceQuotes[4] =
{
    { 26132, "Your essence is MINE!" },
    { 26053, "Your magic is pathetic! Let me show you mine." },
    { 26133, "The shadows serve me now!" },
    { 26048, "Return to the shadows!" },
};

static const SimpleQuote eyesQuotes[7] =
{
    { 26125, "The shadows will not save you!" },
    { 26124, "I WILL find you!" },
    { 26126, "Cower in hiding, heh." },
    { 26122, "I can see you!" }, // If eye found player

    { 26102, "My strength returns!" },
    { 26123, "You hide well, worms. But how long can you delay your doom?" }, // Lazy Eye achievement
    { 26117, "ENOUGH! It is time to end this game!" }, // Endless Rage
};

static const SimpleQuote onKillQuotes[2] =
{
    { 26130, "You lose." },
    { 26131, "None compare to Peroth'arn." },
};

static const SimpleQuote onDeathQuotes[3] =
{
    { 26113, "Noooo! How could this be?" },
    { 26051, "The hunter became the prey." },
    { 26052, "You did well, but for now I must continue alone. Good hunting." },
};

enum Spells
{
    SPELL_CAMOUFLAGE = 105341,
    SPELL_REMOVE_CAMOUFLAGE = 105541,

    SPELL_FEL_ADDLED = 105545, // 6s stun -> huge radius !!! be aware

    SPELL_CORRUPTING_TOUCH = 104939,
    SPELL_FEL_FLAMES_MISSILE = 108141,
    SPELL_FEL_FLAMES_PERIODIC = 108214,
    SPELL_FEL_DECAY = 105544,
    SPELL_FEL_DECAY_HEAL_AURA = 108124, // trigered by spell above
    SPELL_FEL_SURGE = 108128, // Damage spell when healing
    SPELL_DRAIN_ESSENCE = 104905,
    SPELL_EASY_PREY = 105493, // stun player for 8 seconds
    SPELL_FEL_QUICKENING = 105526, // gain 100 % AS
    SPELL_ENFEEBLED = 105442, // self stun
    SPELL_ENDLESS_FRENZY = 105521,

    SPELL_TARGET_LOCKED = 105496, // Tracked Lock on Player (charge and trigger fel quickening ? )

    // ILLIDAN SPELLS
    SPELL_ATTACK_ME_PEROTHARN = 105509, // Taunt spell
    SPELL_ABSORB_FEL_ENERGY = 105543, // triggering 105546 -> TARGET_UNIT_NEARBY_ENTRY !!!
    SPELL_ILLIADAN_MEDITATION = 105547, // stun + regenerate health
    SPELL_REMOVE_MEDITATION = 105548, // don't work ?
};

enum Phase
{
    PHASE_ONE,
    PHASE_HIDE_AND_SEEK
};

enum PerotharnActions
{
    ACTION_NONE,
    ACTION_ESSENCE_START,
    ACTION_ILLIADAN_AGGRO
};

enum CreatureEntries
{
    ENTRY_PEROTHARN = 55085,
    ENTRY_FEL_FLAME = 57329,

    ENTRY_EYE_OF_PEROTHARN = 55868,

    ENTRY_ILLIDAN = 55500, // 55532 also very similar entry

    ENTRY_HUNTING_SUMMONER = 56248,
    ENTRY_HUNTING_CIRCLE_PUFFER = 56182,
};

enum GameObjectEntries
{
    // TODO
};

enum PerotharnWP
{
    WP_MIDDLE_INTRO = 0,
    WP_INTRO_END = 1
};

static void DespawnNearbyCreaturesWithEntry(Creature * source, uint32 entry)
{
    std::list<Creature*> creatures;

    source->GetCreatureListWithEntryInGrid(creatures, entry, 500.0f);

    for (std::list<Creature*>::iterator itr = creatures.begin(); itr != creatures.end(); ++itr)
        (*itr)->ForcedDespawn();
}

typedef std::list<uint64> GoSummonList;

class boss_perotharn : public CreatureScript
{
public:
    boss_perotharn() : CreatureScript("boss_perotharn") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_perotharnAI(creature);
    }

    struct boss_perotharnAI : public ScriptedAI
    {
        boss_perotharnAI(Creature* creature) : ScriptedAI(creature), Summons(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            felGuardGUID = 0;
            goList.clear();
            pInstance = creature->GetInstanceScript();

            me->SummonGameObject(FIREWALL_INVIS_ENTRY, 3226.0f, -4981.0f, 194.2f, 3.86f, 0, 0, 0, 0, 0);
            SummonIntroCreatures();

            introTimer = 30000;
            introStep = 0;
        }

        void SummonIntroCreatures()
        {
            if (Creature * felgurad = me->SummonCreature(LEGION_DEMON_ENTRY, 3182.65f, -4939.93f, START_Z_COORD, 5.4f))
                felGuardGUID = felgurad->GetGUID();

            me->SummonCreature(CORRUPTED_ARCANIST_ENTRY, 3200.54f, -4945.15f, START_Z_COORD, 2.02f);

            me->SummonCreature(DREADLORD_DEFFENDER_ENTRY, 3206.92f, -4948.82f, START_Z_COORD, 2.38f);
            me->SummonCreature(DREADLORD_DEFFENDER_ENTRY, 3198.35f, -4952.23f, START_Z_COORD, 1.93f);

            if (Creature * pGuard = me->SummonCreature(GUARDIAN_DEMON_ENTRY, 3332.8f, -4918.25f, COURTYARD_Z, 1.16f))
                pGuard->AI()->SetData(DATA_SET_GUARDIAN_LANE, WAVE_ONE);
            if (Creature * pGuard = me->SummonCreature(GUARDIAN_DEMON_ENTRY, 3367.87f, -4891.7f, COURTYARD_Z, 3.0f))
                pGuard->AI()->SetData(DATA_SET_GUARDIAN_LANE, WAVE_TWO);
            if (Creature * pGuard = me->SummonCreature(GUARDIAN_DEMON_ENTRY, 3263.63f, -4868.42f, COURTYARD_Z, 5.7f))
                pGuard->AI()->SetData(DATA_SET_GUARDIAN_LANE, WAVE_THREE);
        }

        InstanceScript* pInstance;
        SummonList Summons;
        GoSummonList goList;
        Phase phase;

        // Timers
        uint32 felFlamesTimer;
        uint32 felDecayTimer;
        uint32 essenceTimer;
        uint32 seekTimer;
        uint32 freeIllidanTimer;
        uint8 essenceStep;
        uint32 seekStep;

        // Intro stuff
        uint64 felGuardGUID;
        uint32 introTimer;
        uint32 introStep;
        uint32 aggroTimer;

        bool canMoveToNextPoint;
        bool essenceUsed;
        bool frenzyUsed;

        void JustSummoned(Creature* pSummoned) override
        {
            Summons.Summon(pSummoned);
        }

        void Reset() override
        {
            seekStep = 0;
            phase = PHASE_ONE;
            canMoveToNextPoint = false;
            essenceUsed = false;
            frenzyUsed = false;

            if (pInstance)
            {
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                pInstance->SetData(DATA_PEROTHARN, NOT_STARTED);
            }

            felFlamesTimer = 5000;
            felDecayTimer = 8000;
            aggroTimer = NEVER;
            seekTimer = NEVER;
            freeIllidanTimer = NEVER;

            me->CastSpell(me, SPELL_CORRUPTING_TOUCH, true);
            Summons.DespawnAll();
        }

        uint32 GetData(uint32 type) override
        {
            if (type == DATA_GET_PEROTHARN_PHASE)
                return phase;

            return 0;
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_ESSENCE_START)
            {
                seekTimer = 4005;
                phase = PHASE_HIDE_AND_SEEK;
                // TODO: set fel flame + fel decay timers ...
                me->InterruptNonMeleeSpells(false);
                PlayQuote(me,essenceQuotes[0]);
                me->CastSpell(me, SPELL_DRAIN_ESSENCE, false); // pacify + damage
            }
            else if (action == ACTION_PEROTHRAN_PREPARE_TO_AGGRO)
            {
                PlayQuote(me,eventStart);
                me->RemoveAura(SPELL_CAMOUFLAGE);
                aggroTimer = 4500;
            }
            else if (action == ACTION_HIDE_AND_SEKK_END)
            {
                Summons.DespawnEntry(ENTRY_EYE_OF_PEROTHARN);
                Summons.DespawnEntry(ENTRY_HUNTING_SUMMONER);
                Summons.DespawnEntry(ENTRY_HUNTING_CIRCLE_PUFFER);

                freeIllidanTimer = 3500;

                me->RemoveAllAuras();
                me->CastSpell(me, SPELL_CORRUPTING_TOUCH, true);
                me->GetMotionMaster()->Clear(false);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->GetMotionMaster()->MoveChase(me->GetVictim());
                phase = PHASE_ONE;
            }
        }

        void HandleFireWallCircle(bool spawn)
        {
            if (spawn)
            {
                float x, y, z;
                float angle;
                const uint32 MAX_WALLS = 17;
                for (uint32 i = 0; i < MAX_WALLS; i++)
                {
                    angle = ((2 * M_PI) / MAX_WALLS) * float(i);
                    angle = MapManager::NormalizeOrientation(angle);

                    me->GetNearPoint(me, x, y, z, 58.0f, 0, angle);
                    if (GameObject * go = me->SummonGameObject(FIREWALL_GO_ENTRY, x, y, z, MapManager::NormalizeOrientation(angle + M_PI/3.5f + 0.45f), 0.0f, 0, 0, 0, 0))
                        goList.push_back(go->GetGUID());
                }
            }
            else
            {
                // Remove walls
                for (auto it = goList.begin(); it != goList.end(); it++)
                    if (GameObject * goWall = ObjectAccessor::GetGameObject(*me, *it))
                        goWall->Delete();
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            HandleFireWallCircle(true);

            DespawnNearbyCreaturesWithEntry(me, GUARDIAN_DEMON_ENTRY);
            Summons.DespawnAll();

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            PlayQuote(me,aggroQuotes[0]);
            DoAction(ACTION_ILLIADAN_AGGRO);
            if (pInstance)
            {
                CAST_WOE_INSTANCE(pInstance)->RegisterIllidanVictim(me->GetGUID());
                pInstance->SetData(DATA_PEROTHARN, IN_PROGRESS);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }
        }

        void AttackStart(Unit * victim) override
        {
            // First remove passenger !
            if (Creature* vehPassenger = victim->GetVehicleCreatureBase())
            {
                vehPassenger->Kill(vehPassenger);
                vehPassenger->ForcedDespawn();
            }

            victim->RemoveAura(102994); // Remove Stealth + vehicle kit

            ScriptedAI::AttackStart(victim);
        }

        void EnterEvadeMode() override
        {
            HandleFireWallCircle(false);
            me->SetReactState(REACT_AGGRESSIVE);
            ScriptedAI::EnterEvadeMode();

            if (Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN, 200.0f, true))
            {
                pIllidan->AI()->DoAction(ACTION_ILLIDAN_REMOVE_VEHICLE);
                pIllidan->RemoveAllAuras();
            }

            Summons.DespawnEntry(ENTRY_EYE_OF_PEROTHARN);
            Summons.DespawnEntry(ENTRY_HUNTING_SUMMONER);
            Summons.DespawnEntry(ENTRY_HUNTING_CIRCLE_PUFFER);

            if (pInstance)
            {
                CAST_WOE_INSTANCE(pInstance)->UnRegisterIllidanVictim(me->GetGUID());
                pInstance->SetData(DATA_PEROTHARN, NOT_STARTED);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                PlayQuote(me,onKillQuotes[urand(0,1)]);
            }
        }

        void JustDied(Unit * /*killer*/) override
        {
            HandleFireWallCircle(false);
            Summons.DespawnAll();
            PlayQuote(me,onDeathQuotes[urand(0,2)]);

            if (pInstance)
            {
                CAST_WOE_INSTANCE(pInstance)->UnRegisterIllidanVictim(me->GetGUID());
                pInstance->SetData(DATA_PEROTHARN, DONE);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == WP_MIDDLE_INTRO)
            {
                canMoveToNextPoint = true;
            }
            else if (id == WP_INTRO_END)
            {
                me->SetHomePosition(COURTYARD_X, COURTYARD_Y, COURTYARD_Z, 2.35f);
                me->SetFacingTo(2.35f);
                me->setFaction(35); // for sure
                me->CastSpell(me, SPELL_CAMOUFLAGE, true);
                me->RemoveAura(SPELL_CORRUPTING_TOUCH);
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (canMoveToNextPoint)
            {
                canMoveToNextPoint = false;
                me->GetMotionMaster()->MovePoint(1, COURTYARD_X, COURTYARD_Y, COURTYARD_Z, true);
            }

            if (introStep < 5 && introTimer <= diff)
            {
                if (introStep < 3)
                    PlayQuote(me,introQuotes[introStep]);
                else
                {
                    if (introStep == 4)
                    {
                        introStep++;
                        if (Creature * pLegionDemon = ObjectAccessor::GetCreature(*me, felGuardGUID))
                            pLegionDemon->GetMotionMaster()->MovePoint(1, 3197.25f, -4942.93f, START_Z_COORD, true);
                        return;
                    }

                    me->GetMotionMaster()->MovePoint(0, ESCAPE_X, ESCAPE_Y, ESCAPE_Z, true);

                    if (Creature * pArcanist = me->FindNearestCreature(CORRUPTED_ARCANIST_ENTRY,50.0f,true))
                        pArcanist->GetMotionMaster()->MovePoint(0, ESCAPE_X, ESCAPE_Y, ESCAPE_Z, true);

                    std::list<Creature*> creatures;
                    me->GetCreatureListWithEntryInGrid(creatures, DREADLORD_DEFFENDER_ENTRY, 50.0f);
                    for (std::list<Creature*>::iterator itr = creatures.begin(); itr != creatures.end(); ++itr)
                        (*itr)->GetMotionMaster()->MovePoint(0, ESCAPE_X, ESCAPE_Y, ESCAPE_Z, true);
                }

                introTimer = 5000;
                introStep++;

                if (introStep == 3)
                    introTimer = 2000;
                else if (introStep == 4)
                    introTimer = 3000;
            }
            else introTimer -= diff;

            // ILLIDAN FINISHED INTRO -> start encounter
            if (aggroTimer <= diff)
            {
                if (pInstance)
                {
                    // Remove stealth auras
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(102994); // TODO: is this safe ? -> we should kill vehicle ...
                    if (Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN, 200.0f, true))
                    {
                        pIllidan->AI()->DoAction(ACTION_ILLIDAN_REMOVE_VEHICLE); // Need to first properly remove vehicle passenger and destroy vehicle kit
                        pIllidan->RemoveAllAuras();
                    }
                }
                me->setFaction(14);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();
                me->CastSpell(me,SPELL_CORRUPTING_TOUCH, true);
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                aggroTimer = NEVER;
            }
            else aggroTimer -= diff;

            if (!UpdateVictim())
                return;

            if (phase == PHASE_HIDE_AND_SEEK)
            {
                Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN, 200.0f, true);

                if (seekTimer <= diff)
                {
                    if (seekStep == 0) // Illidan interrupts Perotharn
                    {
                        me->GetMotionMaster()->MoveIdle();
                        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                        pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DRAIN_ESSENCE);
                        me->CastSpell(me, SPELL_FEL_ADDLED, true); // 6s stun to players

                        me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ABSORB_FEL_ENERGY, false); // For sure -> casue it is stun
                        if (pIllidan)
                        {
                            PlayQuote(pIllidan,essenceQuotes[1]); // make it to illidan (pathetic quote)
                            pIllidan->CastSpell(me, SPELL_ABSORB_FEL_ENERGY, false);
                        }

                        seekStep++;
                        seekTimer = 6500;
                    }
                    else if (seekStep == 1)
                    {
                        if (pIllidan)
                            me->SetFacingTo(me->GetAngle(pIllidan));
                        me->RemoveAllAuras(); // Remove corrupting touch
                        me->CastSpell(me, SPELL_CAMOUFLAGE, true);
                        me->GetMotionMaster()->MoveIdle();
                        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
                        me->MonsterTextEmote("Peroth'arn vanishes into to shadows!", LANG_UNIVERSAL, true);
                        PlayQuote(me,essenceQuotes[2]); // The shadows serve me now !
                        seekStep++;
                        seekTimer = 2500;
                    }
                    else if (seekStep == 2)
                    {
                        if (pIllidan)
                        {
                            PlayQuote(pIllidan,essenceQuotes[3]); // Return to the shadows.
                            pIllidan->CastSpell(pIllidan, SPELL_ILLIADAN_MEDITATION, true);
                            // trigger shadows on players again ...
                        }
                        seekStep++;
                        seekTimer = 4000;
                    }
                    else if (seekStep == 3)
                    {
                        if (pIllidan)
                            PlayQuote(pIllidan, aggroQuotes[1]); // Nothing will stop me! Not even you, Demon!

                        seekStep++;
                        seekTimer = 4500;
                    }
                    else if (seekStep == 4)
                    {
                        PlayQuote(me, eyesQuotes[1]);
                        me->MonsterTextEmote("The Eyes of Peroth'arn are looking for you.", LANG_UNIVERSAL, true);
                        // Summon our eye summoner, he will take care of everthing ...
                        me->SummonCreature(ENTRY_HUNTING_SUMMONER, MIDDLE_X, MIDDLE_Y, MIDDLE_Z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 42000);
                        seekTimer = NEVER;
                    }
                }
                else seekTimer -= diff;

                return;
            }

            // PHASE_ONE STUFF FROM HERE

            // Fel Flame
            if (felFlamesTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                        me->CastSpell(target, SPELL_FEL_FLAMES_MISSILE, false);
                    felFlamesTimer = 8400;
                }
            }
            else felFlamesTimer -= diff;

            // Decay
            if (felDecayTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                        me->CastSpell(target, SPELL_FEL_DECAY, false);
                    felFlamesTimer = 8400;
                }

                felDecayTimer = 10000;
            }
            else felDecayTimer -= diff;

            if (freeIllidanTimer <= diff)
            {
                if (Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN, 200.0f, true))
                {
                    pIllidan->AI()->DoAction(ACTION_ILLIDAN_REMOVE_VEHICLE);
                    pIllidan->RemoveAllAuras();
                    PlayQuote(pIllidan, eyesQuotes[4]);
                }
                freeIllidanTimer = NEVER;
            }
            else freeIllidanTimer -= diff;

            if (HealthBelowPct(70) && essenceUsed == false)
            {
                me->AI()->DoAction(ACTION_ESSENCE_START);
                DespawnNearbyCreaturesWithEntry(me, ENTRY_FEL_FLAME);
                essenceUsed = true;
            }

            if (HealthBelowPct(20) && frenzyUsed == false)
            {
                me->CastSpell(me, SPELL_ENDLESS_FRENZY, true);
                PlayQuote(me, eyesQuotes[6]);
                frenzyUsed = true;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_fel_flames_WoE : public CreatureScript
{
public:
    npc_fel_flames_WoE() : CreatureScript("npc_fel_flames_WoE") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fel_flames_WoEAI(creature);
    }

    struct npc_fel_flames_WoEAI : public ScriptedAI
    {
        npc_fel_flames_WoEAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            instance = creature->GetInstanceScript();

            if (instance) // Fel flames is summoned via missile, so it spawns with delay and we dont want them in second phase
            if (Creature* pPerotharn = Unit::GetCreature(*me, instance->GetData64(DATA_PEROTHARN)))
            if (pPerotharn->AI()->GetData(DATA_GET_PEROTHARN_PHASE) == 1)
                me->ForcedDespawn();

            me->CastSpell(me, SPELL_FEL_FLAMES_PERIODIC, true);
        }

        InstanceScript* instance;

        void Reset() override {}
        void EnterCombat(Unit * /*who*/)  override {}
        void EnterEvadeMode() override {}

        void KilledUnit(Unit* victim) override
        {
            if (!instance)
                return;

            if (Creature* pPerotharn = Unit::GetCreature(*me, instance->GetData64(DATA_PEROTHARN)))
                pPerotharn->AI()->KilledUnit(victim);
        }

        void UpdateAI(const uint32 diff) override {}
    };
};

enum huntingSpells
{
    SPELL_HUNTING_CHANNELING = 105353,
    SPELL_HUNTING_PUFF = 105463, // :P
    SPELL_HUNTING_PULSE = 105379 , // explosion like
};

class npc_eye_of_perotharn_woe : public CreatureScript
{
public:
    npc_eye_of_perotharn_woe() : CreatureScript("npc_eye_of_perotharn_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_of_perotharn_woeAI(creature);
    }

    struct npc_eye_of_perotharn_woeAI : public ScriptedAI
    {
        npc_eye_of_perotharn_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->CastSpell(me, SPELL_HUNTING_PUFF, true);
            pInstance = me->GetInstanceScript();
            me->SetSpeed(MOVE_WALK, 0.6f, true);
            me->SetSpeed(MOVE_RUN, 0.6f, true);
            firstMove = true;
        }

        InstanceScript * pInstance;
        uint32 moveTimer;
        float startAngle;
        float endAngle;
        bool firstMove;

        void Reset() override
        {
            startAngle = endAngle = 0.0f;
            moveTimer = NEVER;
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void SetSearchingAngles(float angle_from, float angle_to)
        {
            startAngle = angle_from;
            endAngle = angle_to;

            if (endAngle > startAngle)
                std::swap(startAngle, endAngle);

            moveTimer = 100;
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id != 1)
                return;

            moveTimer = 1;
        }

        void MoveToNextPoint()
        {
            float distance = frand(25.0f, 62.0f);
            if (firstMove)
            {
                distance = frand(50.0f, 62.0f);
                firstMove = false;
            }

            float angle = MapManager::NormalizeOrientation(frand(startAngle, endAngle));
            // This is very special case, if angle is from 0.5 to 5.5 -> this means angle was rounded via normalization
            // and we really want rand angle from <5.5,0.0X> but that is not supported from PRNG
            if (endAngle > 5.5f)
                endAngle = frand(5.5f, 2 * M_PI);
            float x = MIDDLE_X +cos(angle) * distance;
            float y = MIDDLE_Y +sin(angle) * distance;
            float z = me->GetPositionZ();
            me->GetMotionMaster()->MovePoint(1, x, y, z, true, false);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (me->GetExactDist2d(who) > 5.0f || who->GetTypeId() != TYPEID_PLAYER)
                return;

            ScriptedAI::MoveInLineOfSight(who);
        }

        void EnterCombat(Unit * who) override
        {
            if (pInstance == NULL || who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (Creature * pPerotharn = ObjectAccessor::GetCreature(*me,pInstance->GetData64(DATA_PEROTHARN)))
            {
                pPerotharn->AI()->DoAction(ACTION_HIDE_AND_SEKK_END);
                who->CastSpell(pPerotharn, SPELL_ATTACK_ME_PEROTHARN, true); // Taunt Perotharn
                pPerotharn->GetMotionMaster()->Clear(false);
                pPerotharn->GetMotionMaster()->MoveChase(who);
                pPerotharn->CastSpell(who, SPELL_TARGET_LOCKED, true); // charge to victim (working ?)
                who->CastSpell(who, SPELL_EASY_PREY, true); // stun player
                pPerotharn->CastSpell(pPerotharn, SPELL_FEL_QUICKENING, true); // gain attack speed
                pPerotharn->AI()->AttackStart(who);
                PlayQuote(pPerotharn, eyesQuotes[3]);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (moveTimer <= diff)
            {
                MoveToNextPoint();
                moveTimer = NEVER; // next move will be set when current WP will be reached
            }
            else moveTimer -= diff;
        }
    };
};

class npc_hunting_summoner_woe : public CreatureScript
{
public:
    npc_hunting_summoner_woe() : CreatureScript("npc_hunting_summoner_woe") { }

    CreatureAI* GetAI(Creature* creature) const 
    {
        return new npc_hunting_summoner_woeAI(creature);
    }

    struct npc_hunting_summoner_woeAI : public ScriptedAI
    {
        npc_hunting_summoner_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_HUNTING_PULSE, true);
            me->CastSpell(me, SPELL_HUNTING_CHANNELING, false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);

            for (uint32 i = 0; i < 8; i++)
                positions.push_back(i);
        }

        InstanceScript * pInstance;
        uint32 summonPufferTimer;
        uint32 eyeQuoteTimer;
        uint32 summonCounter;

        //std::vector<uint32> positions = {0,1,2,3,4,5,6,7}; -> not working on my compiler for now ( MS VS u sux)
        std::vector<uint32> positions;

        void Reset() override
        {
            summonCounter = 0;
            summonPufferTimer = 1000;
            eyeQuoteTimer = 3000;
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (!me->HasAura(SPELL_HUNTING_CHANNELING))
            {
                if (Creature * pPerotharn = ObjectAccessor::GetCreature(*me,pInstance->GetData64(DATA_PEROTHARN)))
                {
                    PlayQuote(pPerotharn, eyesQuotes[5]); // Lazy eye quote
                    pInstance->DoCompleteAchievement(6127); // Lazy eye achiev
                    pPerotharn->AI()->DoAction(ACTION_HIDE_AND_SEKK_END); // this should despawn us also ...
                }
                return;
            }

            if (summonPufferTimer <= diff)
            {
                float angle = M_PI / 4;
                float x, y, z;

                uint32 vectorSize = positions.size();
                uint32 rNum = urand(0, vectorSize - 1);
                angle = angle * positions[rNum];
                positions.erase(positions.begin() + rNum);

                me->GetNearPoint(me, x, y, z, 7.0f, 0.0f, angle);
                if (Creature * pPerotharn = ObjectAccessor::GetCreature(*me,pInstance->GetData64(DATA_PEROTHARN)))
                    pPerotharn->SummonCreature(ENTRY_HUNTING_CIRCLE_PUFFER, x, y, z, angle, TEMPSUMMON_TIMED_DESPAWN, 40000 - (vectorSize * 200));

                summonPufferTimer = positions.empty() ? NEVER : 200;
            }
            else summonPufferTimer -= diff;

            if (eyeQuoteTimer <= diff)
            {
                if (Creature * pPerotharn = me->FindNearestCreature(PEROTHARN_ENTRY,250.0f,true))
                {
                    if (summonCounter == 15)
                        PlayQuote(pPerotharn, eyesQuotes[0]);
                    else if (summonCounter == 30)
                        PlayQuote(pPerotharn, eyesQuotes[2]);
                }
                summonCounter++;
                eyeQuoteTimer = 500;
            }
            else eyeQuoteTimer -= diff;
        }
    };
};

class npc_hunting_puffer_woe : public CreatureScript
{
public:
    npc_hunting_puffer_woe() : CreatureScript("npc_hunting_puffer_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_hunting_puffer_woeAI(creature);
    }

    struct npc_hunting_puffer_woeAI : public ScriptedAI
    {
        npc_hunting_puffer_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            summonEyeTimer = 1500;
            pInstance = me->GetInstanceScript();
        }

        uint32 summonEyeTimer;
        InstanceScript * pInstance;

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (summonEyeTimer <= diff)
            {
                float ori = me->GetOrientation();
                Creature * pEye = NULL;

                if (Creature * pPerotharn = ObjectAccessor::GetCreature(*me,pInstance->GetData64(DATA_PEROTHARN)))
                    pEye = pPerotharn->SummonCreature(ENTRY_EYE_OF_PEROTHARN, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), ori);

                if (pEye)
                {
                    if (npc_eye_of_perotharn_woe::npc_eye_of_perotharn_woeAI* pAI = (npc_eye_of_perotharn_woe::npc_eye_of_perotharn_woeAI*)(pEye->GetAI()))
                        pAI->SetSearchingAngles(ori, MapManager::NormalizeOrientation(ori + M_PI /4));
                }
                summonEyeTimer = 8000;
            }
            else summonEyeTimer -= diff;
        }
    };
};

void AddSC_boss_perotharn()
{
    new boss_perotharn(); // 55085
    new npc_fel_flames_WoE(); // 57329
    new npc_hunting_summoner_woe(); // 56248
    new npc_hunting_puffer_woe(); // 56182
    new npc_eye_of_perotharn_woe(); // 55868
}