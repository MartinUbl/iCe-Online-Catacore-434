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

typedef bool QuoteOwner;

const QuoteOwner PEROTHARN_QUOTE = true;
const QuoteOwner ILLIADAN_QUOTE = false;

struct Quotes
{
    QuoteOwner quoteOwner;
    uint32 soundId;
    const char * text;
};

static const Quotes introQuotes[3] =
{
    { PEROTHARN_QUOTE, 26118, "He is near, lurking in the shadows... I can sense it." },
    { PEROTHARN_QUOTE, 26120, "You, Felguard. Hold this area." },
    { PEROTHARN_QUOTE, 26121, "The rest of you, secure the courtyard." },
};

static const Quotes portalQuotes[6] =
{
    { ILLIADAN_QUOTE, 26073, "The demons should all be leaving. We'll be in the palace in no time" },
    { PEROTHARN_QUOTE, 26127, "Who shut down the portals!? Clever. Little worms." },
    { ILLIADAN_QUOTE, 26074, "The demons are no longer pouring from the palace. We can move ahead." },
    { ILLIADAN_QUOTE, 26075, "Too easy." },
    { ILLIADAN_QUOTE, 26050, "Another demon ready to be slaughtered" },
    { PEROTHARN_QUOTE, 26128, "None will reach the palace without besting Peroth'arn...first of the feltouched" },
};

static const Quotes aggroQuotes[2] =
{
    { PEROTHARN_QUOTE, 26112, "No mortal may stand before me and live!" },
    { ILLIADAN_QUOTE, 26049, "Nothing will stop me! Not even you, Demon!" },
};

static const Quotes essenceQuotes[4] =
{
    { PEROTHARN_QUOTE, 26132, "Your essence is MINE!" },
    { ILLIADAN_QUOTE, 26053, "Your magic is pathetic! Let me show you mine." },
    { PEROTHARN_QUOTE, 26133, "The shadows serve me now!" },
    { ILLIADAN_QUOTE, 26048, "Return to the shadows!" },
};

static const Quotes eyesQuotes[7] =
{
    { PEROTHARN_QUOTE, 26125, "The shadows will not save you!" },
    { PEROTHARN_QUOTE, 26124, "I WILL find you!" },
    { PEROTHARN_QUOTE, 26126, "Cower in hiding, heh." },
    { PEROTHARN_QUOTE, 26122, "I can see you!" }, // If eye found player

    { ILLIADAN_QUOTE, 26102, "My strength returns!" },
    { PEROTHARN_QUOTE, 26123, "You hide well, worms. But how long can you delay your doom?" }, // Lazy Eye achievement
    { PEROTHARN_QUOTE, 26117, "ENOUGH! It is time to end this game!" }, // Endless Rage
};

static const Quotes onKillQuotes[2] =
{
    { PEROTHARN_QUOTE, 26130, "You lose." },
    { PEROTHARN_QUOTE, 26131, "None compare to Peroth'arn." },
};

static const Quotes onDeathQuotes[3] =
{
    { PEROTHARN_QUOTE, 26113, "Noooo! How could this be?" },
    { ILLIADAN_QUOTE, 26051, "The hunter became the prey." },
    { ILLIADAN_QUOTE, 26052, "You did well, but for now I must continue alone. Good hunting." },
};

enum Spells
{
    SPELL_CAMOUFLAGE = 105341,
    SPELL_REMOVE_CAMOUFLAGE = 105541,

    SPELL_FEL_ADDLED = 105545, // 6s stun -> huge radius !!! be aware

    SPELL_CORRUPTING_TOUCH = 104939,
    SPELL_FEL_FLAMES_MISSILE = 108141,
    SPELL_FEL_FLAMES_AOE = 108214,
    SPELL_FEL_DECAY = 105544,
    SPELL_FEL_DECAY_HEAL_AURA = 108124, // trigered by spell above
    SPELL_FEL_SURGE = 108128, // Damage spell when healing
    SPELL_DRAIN_ESSENCE = 104905, // TODO : change applying aura from pacify to stun
    SPELL_EASY_PREY = 105493, // stun player for 8 seconds
    SPELL_FEL_QUICKENING = 105526, // gain 100 % AS
    SPELL_ENFEEBLED = 105442, // self stun
    SPELL_ENDLESS_FRENZY = 105521,

    // ILLIDAN SPELLS
    SPELL_TAUNT_PEROTHARN = 105509, // Taunt spell
    SPELL_ABSORB_FEL_ENERGY = 105543, // triggering 105546 -> TARGET_UNIT_NEARBY_ENTRY !!!
    SPELL_ILLIADAN_MEDITATION = 105547, // stun + regenerate health
    SPELL_REMOVE_MEDITATION = 105548, // don't work ?


    SPELL_ILLIDAN_GREEN_ARROW = 105924, // visual arrow
    SPELL_SHADOW_CLOAK_CIRCLE = 102951, // visual circle
    SPELL_SHADOW_CLOAK_FAKE = 105915, // visual stealth -> Illidan should not be attackable ?

    // Shadow system on players
    SPELL_SHADOW_WALK_STEALTH = 102994, // stealth + speed + SPELL_AURA_SET_VEHICLE_ID -> misc value 1763)
    SPELL_SHADOW_CLOAK_TRIGGERER = 103004, // Triggering two spells every second (THESE SHOULD TRIGGER SHADOW WALK OUT OF COMBAT)
    SPELL_SHADOW_WALK_STACK_AURA = 103020, // just dummy stacking aura
    SPELL_SHADOW_CLOAK_STEALTH = 110231, // stealth + speed (0 basepoint)
};

enum Phase
{
    PHASE_ONE,
    PHASE_SHADOW,
    PHASE_TWO,
};

enum Actions
{
    ACTION_NONE,
    ACTION_ESSENCE_START,
    ACTION_ILLIADAN_AGGRO,
};

enum CreatureEntries
{
    ENTRY_PEROTHARN = 55085,
    ENTRY_FEL_FLAME = 57329,

    ENTRY_EYE_OF_PEROTHARN = 55868,
    ENTRY_EYE_OF_PEROTHARN_2 = 55879, // hmm second one ? :P

    ENTRY_ILLIDAN_1 = 55532,
    ENTRY_ILLIDAN_2 = 55500,
};

enum GameObjectEntries
{
    // TODO
};

# define NEVER  (0xffffffff)

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
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList Summons;
        Phase phase;

        // Timers and stuff
        uint32 felFlamesTimer;
        uint32 felDecayTimer;

        uint32 essenceTimer;
        uint8 essenceStep;

        bool essenceUsed;
        bool frenzyUsed;

        void JustSummoned(Creature* pSummoned)
        {
            Summons.Summon(pSummoned);
        }

        void Reset()
        {
            felFlamesTimer = 5000;
            felDecayTimer = 8000;

            me->CastSpell(me, SPELL_CORRUPTING_TOUCH, true);
            Summons.DespawnAll();
        }

        void PlayQuote(Quotes q)
        {
            DoPlaySoundToSet(me, q.soundId);
            me->MonsterYell(q.text, LANG_UNIVERSAL, 0);
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_ESSENCE_START)
            {
                me->InterruptNonMeleeSpells(false);
                me->CastSpell(me, SPELL_DRAIN_ESSENCE, false);
            }
        }

        void EnterCombat(Unit * /*who*/)
        {
            PlayQuote(aggroQuotes[0]);
            DoAction(ACTION_ILLIADAN_AGGRO);
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                PlayQuote(onKillQuotes[urand(0,1)]);
            }
        }

        void JustDied()
        {
            Summons.DespawnAll();
            PlayQuote(onDeathQuotes[urand(0,2)]);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // Fel Flame
            if (felFlamesTimer <= diff)
            {
                if (me->IsNonMeleeSpellCasted(false))
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
                if (me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                        me->CastSpell(target, SPELL_FEL_DECAY, false);
                    felFlamesTimer = 8400;
                }

                felDecayTimer = 10000;
            }
            else felDecayTimer -= diff;

            if (HealthBelowPct(70) && essenceUsed == false)
            {
                me->AI()->DoAction(ACTION_ESSENCE_START);
                essenceUsed = true;
            }

            if (HealthBelowPct(20) && frenzyUsed == false)
            {
                me->CastSpell(me, SPELL_ENDLESS_FRENZY, true);
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
            me->CastSpell(me, SPELL_FEL_FLAMES_AOE, true);
        }

        InstanceScript* instance;

        void Reset(){}
        void EnterCombat(Unit * /*who*/) {}
        void EnterEvadeMode() {}

        void KilledUnit(Unit* victim)
        {
            if (!instance)
                return;

            if (Creature* pPerotharn = Unit::GetCreature(*me, instance->GetData64(DATA_PEROTHARN)))
                pPerotharn->AI()->KilledUnit(victim);
        }

        void UpdateAI(const uint32 diff)
        {
        }
    };
};



void AddSC_boss_perotharn()
{
    new boss_perotharn(); // 55085
    new npc_fel_flames_WoE(); // 57329
}