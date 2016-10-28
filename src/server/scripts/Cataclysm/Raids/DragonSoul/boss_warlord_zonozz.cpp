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
 Encounter: Warlord Zon'ozz
 Dungeon: Dragon Soul
 Difficulty: Normal / Heroic
 Mode: 10-man normal/ 10-man heroic
 Autor: Artisan
 Note: 25 normal and 25 heroic NYI
 */

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "MapManager.h"
#include "TaskScheduler.h"

enum spells : uint32
{
    /* BOSS SPELLS */
    SPELL_FOCUSED_ANGER                 = 104543, // OK
    SPELL_FOCUSED_ANGER_25N             = 104543,
    SPELL_FOCUSED_ANGER_10HC            = 104543,
    SPELL_FOCUSED_ANGER_25HC            = 104543,
    SPELL_PSYCHIC_DRAIN                 = 104323, // OK
    SPELL_DISRUPTING_SHADOWS            = 103434, // OK (maybe cast with max affected targets)
    SPELL_DISRUPTING_SHADOWS_KNOCKBACK  = 103948, // this should be triggered after dispeling spell above
    SPELL_BLACK_BLOOD_10_HC             = 104377, // stackable periodic damage aura
    SPELL_BLACK_BLOOD_25_HC             = 110306, // stackable periodic damage aura
    SPELL_BLACK_BLOOD                   = 104378, // higher damage + 30 s duration
    SPELL_VOID_OF_THE_UNMAKING_CHANNEL  = 103946, // dummy channel
    SPELL_VOID_OF_THE_UNMAKING_SUMMON   = 103571, // summon sphere in front of the caster
    SPELL_DARKNESS                      = 109413, // damn, hate looking up correct spells
    SPELL_SUMMON_EYE                    = 109190,
    SPELL_SUMMON_CLAW                   = 109191,
    SPELL_ROAR_EMOTE                    = 34999,  // just borrow this spell
    SPELL_CAMERA_SHAKE                  = 101106, // maybe correct
    SPELL_BERSERK                       = 47008,

    /* VOID SPHERE SPELLS */
    SPELL_VOID_DIFFUSION_DAMAGE         = 103527, // should split damage
    SPELL_VOID_DIFFUSION_STACK          = 106836, // increase damage + scale of sphere -> triggered by spell above
    SPELL_VOID_DIFFUSION_TAKEN          = 104031, // stackable damage taken aura (TARGET_UNIT_SRC_AREA_ENTRY) -> just use AddAura on boss instead
    SPELL_VOID_OF_THE_UNMAKING_VISUAL   = 109187, // visual purple aura (infinite)
    SPELL_VOID_OF_THE_UNMAKING_REMOVE   = 105336, // should remove aura above
    SPELL_BLACK_BLOOD_ERUPTION          = 108799, // if sphere hits the edge of the room

    /* SUMMONS SPELLS*/
    SPELL_OOZE_SPIT                     = 109396, // Casted by Claw of Go'rath when no one in melee range
    SPELL_SHADOW_GAZE                   = 104347, // Casted by Eye of Go'rath to random player
    SPELL_WILD_FLAIL                    = 109199, // Casted be Flail of Go'rath (melee AoE)
    SPELL_SLUDGE_SPEW                   = 110102, // Casted be Flail of Go'rath to random player

    ACHIEVEMENT_HEROIC_ZONOZZ           = 6110,
    ACHIEVEMENT_PING_PONG_CHAMPION      = 6128,
};

enum blackBloodSpells : uint32
{
    SPELL_BLACK_BLOOD_CIRCLE_EYE        = 103932, // added on Eye of Go'rath for visual black circle underneath
    SPELL_BLACK_BLOOD_SPIT_CLAW         = 103704, // spitinng black blood large
    SPELL_BLACK_BLOOD_SPIT_FLAIL        = 109361  // spitinng black blood, smaller one
};

enum entries : uint32
{
    WARLORD_ZON_OZZ_ENTRY               = 55308,
    CLAW_OF_GORATH_ENTRY                = 55418,
    EYE_OF_GORATH_ENTRY                 = 55416,
    FLAIL_OF_GORATH_ENTRY               = 55417,
    VOID_SPHERE_ENTRY                   = 55334,
    VOID_SPHERE_HC_OR_25                = 58473  // not used currently
};

enum whisperSpells : uint32
{
    SPELL_ZON_OZZ_WHISPER_AGGRO   = 109874,
    SPELL_ZON_OZZ_WHISPER_INTRO   = 109875,
    SPELL_ZON_OZZ_WHISPER_DEATH   = 109876,
    SPELL_ZON_OZZ_WHISPER_SLAY    = 109877,
    SPELL_ZON_OZZ_WHISPER_PHASE   = 109878,
    SPELL_ZON_OZZ_WHISPER_SHADOWS = 109879,
    SPELL_ZON_OZZ_WHISPER_VOID    = 109880
};

static const FacelessQuote introQuote =
{ 
    26337,
    "Vwyq agth sshoq'meg N'Zoth vra zz shfk qwor ga'halahs agthu. Uulg'ma, ag qam.",
    "Once more shall the twisted flesh-banners of N'Zoth chitter and howl above the fly-blown corpse of this world. After millennia, we have returned." 
};

static const FacelessQuote aggroQuote = { 26335, "Zzof Shuul'wah. Thoq fssh N'Zoth!", "Victory for Deathwing. For the glory of N'Zoth!" };

static const FacelessQuote blackQuote = { 26344, "N'Zoth ga zyqtahg iilth.", "The will of N'Zoth corrupts you." };

static const FacelessQuote voidQuote =  { 26345, "Gul'kafh an'qov N'Zoth.", "Gaze into the heart of N'Zoth." };

static const FacelessQuote deathQuote = { 26336, "Uovssh thyzz... qwaz...", "To have waited so long... for this..." };

#define MAX_DISRUPTING_QUOTES 3

static const FacelessQuote disruptingQuotes[MAX_DISRUPTING_QUOTES] =
{
    { 26340, "Sk'shgn eqnizz hoq.", "Your fear drives me." },
    { 26342, "Sk'magg yawifk hoq.", "Your suffering strengthens me." },
    { 26343, "Sk'uuyat guulphg hoq.", "Your agony sustains me." },
};
#define MAX_KILL_QUOTES 3

static const FacelessQuote killQuotes[MAX_KILL_QUOTES] =
{
    { 26338, "Sk'tek agth nuq N'Zoth yyqzz.", "Your skulls shall adorn N'Zoth's writhing throne." },
    { 26339, "Sk'shuul agth vorzz N'Zoth naggwa'fssh.", "Your deaths shall sing of N'Zoth's unending glory." },
    { 26341, "Sk'yahf agth huqth N'Zoth qornaus.", "Your souls shall sate N'Zoth's endless hunger."},
};

#define MIDDLE_X (-1766.0f)
#define MIDDLE_Y (-1917.0f)
#define MIDDLE_Z (-226.00f)

#define ROOM_RADIUS (105.0f)

#define EYE_SPAWN_RANGE (73.0f)
#define FLAIL_SPAWN_RANGE (55.0f)

#define EYE_Z (-221.0f)

#define DARK_PHASE_NORMAL_TIMER (30000)
#define TENTACLES_DESPAWN_TIMER (28000)

#define BOUNCE_REQUIRED_FOR_ACHIEV 10

#define EYES_IN_10_HEROIC       5
#define FLAILS_IN_10_HEROIC     2
#define CLAWS_IN_10_HEROIC      1

#define TENTACLES_IN_10_HEORIC (EYES_IN_10_HEROIC + FLAILS_IN_10_HEROIC + CLAWS_IN_10_HEROIC)

#define MAX_EYES_NORMAL 8

static const Position eyePos [MAX_EYES_NORMAL]  =
{
    {-1791.0f, -1989.0f, EYE_Z, 1.51f},
    {-1733.0f, -1985.0f, EYE_Z, 1.99f},
    {-1694.0f, -1940.0f, EYE_Z, 2.95f},
    {-1701.0f, -1883.0f, EYE_Z, 3.57f},
    {-1745.0f, -1845.0f, EYE_Z, 4.50f},
    {-1802.0f, -1849.0f, EYE_Z, 5.30f},
    {-1840.0f, -1894.0f, EYE_Z, 6.03f},
    {-1835.0f, -1951.0f, EYE_Z, 0.48f}
};

enum bossPhase : uint8
{
    NORMAL_PHASE,
    TRANSITION_PHASE,
    DARK_PHASE,
};

enum actions : int32
{
    ACTION_SPHERE_BOUNCE,
    ACTION_SPHERE_HIT_BOSS
};

class boss_warlord_zonozz : public CreatureScript
{
public:
    boss_warlord_zonozz() : CreatureScript("boss_warlord_zonozz") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_warlord_zonozzAI(creature);
    }

    struct boss_warlord_zonozzAI : public ScriptedAI
    {
        boss_warlord_zonozzAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = me->GetInstanceScript();
            introDone = false;
        }

        InstanceScript* instance;
        TaskScheduler scheduler;

        uint32 voidSphereTimer;
        uint32 focusedAngerTimer;
        uint32 psychicDrainTimer;
        uint32 disruptingShadowsTimer;
        uint32 roarTimer;
        uint32 quoteIndex;
        bool introDone;
        SummonList summons;

        // Achiev
        uint32 bounceCount;
        bossPhase phase;

        void Reset() override
        {
            quoteIndex = 0;
            bounceCount = 0;

            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ZONOZZ) != DONE)
                    instance->SetData(TYPE_BOSS_ZONOZZ, NOT_STARTED);

                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10.0f);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            phase = NORMAL_PHASE;

            me->RemoveAllAuras();

            summons.DespawnAll();
            scheduler.CancelAll();

            if (IsHeroic() && instance)
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACK_BLOOD_10_HC);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACK_BLOOD_25_HC);
            }
        }

        void JustSummoned(Creature *pSummon) override
        {
            summons.Summon(pSummon);
        }

        void SummonedCreatureDespawn(Creature *pSummon) override
        {
            summons.Despawn(pSummon);
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_SPHERE_HIT_BOSS)
            {
                // Add 5% damage taken debuff for every bounce of the sphere
                if (Creature * pSphere = me->FindNearestCreature(VOID_SPHERE_ENTRY, 200.0f, true))
                {
                    if (Aura * pAura = pSphere->GetAura(SPELL_VOID_DIFFUSION_STACK))
                    {
                        uint32 stacks = pAura->GetStackAmount();
                        for (uint32 i = 0; i < stacks; i++)
                            me->AddAura(SPELL_VOID_DIFFUSION_TAKEN, me);
                    }
                }

                // Switch to TRANSITION_PHASE after 500 ms
                scheduler.Schedule(Milliseconds(500), [this](TaskContext /*context*/)
                {
                    me->SetReactState(REACT_PASSIVE);
                    me->GetMotionMaster()->MovePoint(0, MIDDLE_X, MIDDLE_Y, MIDDLE_Z);
                    phase = TRANSITION_PHASE;
                });
            }
            else if (action == ACTION_SPHERE_BOUNCE)
            {
                bounceCount++; // achiev counter
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ZONOZZ, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            RunPlayableQuote(aggroQuote);
            me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_AGGRO, true);

            roarTimer               = MAX_TIMER;
            voidSphereTimer         = 6000;
            focusedAngerTimer       = 12000;
            psychicDrainTimer       = 18000;
            disruptingShadowsTimer  = 26000;

            me->SetInCombatWithZone();

            scheduler.Schedule(Minutes(6), [this](TaskContext)
            {
                me->CastSpell(me, SPELL_BERSERK, true);
            });
        }

        void EnterEvadeMode() override
        {
            if (instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                quoteIndex = urand(0, MAX_KILL_QUOTES - 1);
                RunPlayableQuote(killQuotes[quoteIndex]);
                me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_SLAY, true);
            }
        }

        void JustDied(Unit * killer) override
        {
            RunPlayableQuote(deathQuote);
            me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_DEATH, true);

            if (instance)
            {
                instance->SetData(TYPE_BOSS_ZONOZZ, DONE);

                if (bounceCount >= BOUNCE_REQUIRED_FOR_ACHIEV)
                    instance->DoCompleteAchievement(ACHIEVEMENT_PING_PONG_CHAMPION); // Ping Pong Champion

                if (IsHeroic())
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_ZONOZZ);

                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }

            summons.DespawnAll();

            if (instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (IsHeroic())
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACK_BLOOD_10_HC);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_BLACK_BLOOD_25_HC);
                }
            }

            ScriptedAI::JustDied(killer);
        }

        void MoveInLineOfSight(Unit * who) override
        {
            if (!introDone && who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && who->GetExactDist2d(me) < 60.0f)
            {
                RunPlayableQuote(introQuote);
                me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_INTRO, true);

                introDone = true;
                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            // Remove all stacks of focused anger on boss after arriving to middle
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_25N);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_10HC);
            me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER_25HC);
            // Root him at place
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            // Switch to dark phase
            phase = DARK_PHASE;

            // Black blood phase is always delayed by 1 seconds after arrival to position
            scheduler.Schedule(Seconds(1), [this](TaskContext /*context*/)
            {
                me->CastSpell(me, SPELL_DARKNESS, true);
                RunPlayableQuote(blackQuote);
                me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_PHASE, true);

                roarTimer = 500;

                IsHeroic() ? HandleBlackBloodPhaseHeroic() : HandleBlackBloodPhaseNormal();
            })
            // After roaring for 14 seconds, keep attacking highest threat target
             .Schedule(Seconds(14), [this](TaskContext /*context*/)
            {
                phase = NORMAL_PHASE;
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);

                if (Unit * pVictim = me->GetVictim())
                {
                    me->SetUInt64Value(UNIT_FIELD_TARGET, pVictim->GetGUID());
                    me->GetMotionMaster()->MoveChase(pVictim);
                }

                // We need to setup timers here, beacause these timer are updating only in NORMAL_PHASE
                if (IsHeroic())
                {
                    focusedAngerTimer = 24000;
                    disruptingShadowsTimer = 24000;
                    voidSphereTimer = 40000;
                    psychicDrainTimer = 50000;
                }
                else
                {
                    focusedAngerTimer = 35000;
                    disruptingShadowsTimer = 24000;
                    voidSphereTimer = 24000;
                    psychicDrainTimer = 30000;
                }
            });
        }

        void SpellHitTarget(Unit *target, const SpellEntry *spell) override
        {
            switch (spell->Id)
            {
                case SPELL_ZON_OZZ_WHISPER_INTRO:
                    me->MonsterWhisper(introQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_AGGRO:
                    me->MonsterWhisper(aggroQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_DEATH:
                    me->MonsterWhisper(deathQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_SLAY:
                    if (quoteIndex < MAX_KILL_QUOTES)
                        me->MonsterWhisper(killQuotes[quoteIndex].GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_PHASE:
                    me->MonsterWhisper(blackQuote.GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_SHADOWS:
                    if (quoteIndex < MAX_DISRUPTING_QUOTES)
                        me->MonsterWhisper(disruptingQuotes[quoteIndex].GetWhisperText(), target->GetGUID());
                    break;
                case SPELL_ZON_OZZ_WHISPER_VOID:
                    me->MonsterWhisper(voidQuote.GetWhisperText(), target->GetGUID());
                    break;
                default:
                    break;
            }
        }

        void HandleBlackBloodPhaseNormal()
        {
            me->CastSpell(me, SPELL_BLACK_BLOOD, true);

            uint32 rands[] = { 0,1,2,3,4,5,6,7 };
            std::vector<uint32> randInts(rands, rands + sizeof(rands) / sizeof(uint32));
            std::random_shuffle(randInts.begin(), randInts.end());

            // Summon 5 eyes in 10 man -> 8 eyes in 25 man
            uint32 maxSummons = Is25ManRaid() ? 8 : 5;

            for (uint32 i = 0; i < maxSummons; i++)
                me->SummonCreature(EYE_OF_GORATH_ENTRY, eyePos[i], TEMPSUMMON_TIMED_DESPAWN, TENTACLES_DESPAWN_TIMER);

            float x, y, z;

            //Summon one claw
            me->GetNearPoint(me, x, y, z, 1.0f, frand(20.0f, 30.0f), frand(0.0f, 2.0f * M_PI));
            me->SummonCreature(CLAW_OF_GORATH_ENTRY, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, TENTACLES_DESPAWN_TIMER);

            //Summon one flail
            me->GetNearPoint(me, x, y, z, 1.0f, frand(40.0f, 50.0f), frand(0.0f, 2.0f * M_PI));
            me->SummonCreature(FLAIL_OF_GORATH_ENTRY, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, TENTACLES_DESPAWN_TIMER);
        }

        void HandleBlackBloodPhaseHeroic()
        {
            for (int i = 0; i < TENTACLES_IN_10_HEORIC; i++)
                me->CastSpell(me, SPELL_BLACK_BLOOD_10_HC, true);

            SpawnTentaclesHeroic();
        }

        inline bool IsCastingAllowed()
        {
            return !me->IsNonMeleeSpellCasted(false);
        }

        void SpawnVoidSphere()
        {
            bounceCount = 0;
            RunPlayableQuote(voidQuote);
            me->CastSpell(me, SPELL_ZON_OZZ_WHISPER_VOID, true);

            // Summon void sphere in front of the boss and cast beam on it, we should use SPELL_VOID_OF_THE_UNMAKING_SUMMON, but spawning position of this spell is fucked up
            float x, y, z;
            me->GetNearPoint(me, x, y, z, 30.0f, 0.0f, me->GetOrientation());

            if (Creature * sphere = me->SummonCreature(VOID_SPHERE_ENTRY, x, y, z, me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                me->CastSpell(sphere, SPELL_VOID_OF_THE_UNMAKING_CHANNEL, false);
        }

        /*
            Brief description of this method:
            - eye tentacles should spawn on "holes" in room, there are 8 holes around rooom with same distance with each other
            but only 4 should spawn on heroic
            - eye spawn postions should adjust relative to players position
            - so we need to have find 4 holes following continuously
            - We need to determine M_PI segment which contains most players and spawn tentacles in this semicirlce
        */
        void SpawnTentaclesHeroic()
        {
            float startingOrientation = 5.20f;

            Position pos;
            pos.m_positionX = MIDDLE_X;
            pos.m_positionY = MIDDLE_Y;
            pos.m_positionZ = MIDDLE_Z;

            float MOST_PLAYERS_ANGLE = startingOrientation;
            uint32 MAX_PLAYERS_IN_ARC = 0;

            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();

            for (uint32 segmentIndex = 0; segmentIndex < 8; segmentIndex++)
            {
                float o = MapManager::NormalizeOrientation(startingOrientation + (M_PI / 4.0f * (float)segmentIndex));

                pos.m_orientation = o;
                uint32 playersInAngle = 0;

                for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
                {
                    if (Player* pPlayer = i->getSource())
                    {
                        if (pos.HasInArc(M_PI / 2.0f, pPlayer))
                            playersInAngle++;
                    }
                }

                if (playersInAngle >= MAX_PLAYERS_IN_ARC)
                {
                    MAX_PLAYERS_IN_ARC = playersInAngle;
                    MOST_PLAYERS_ANGLE = MapManager::NormalizeOrientation(o - (M_PI / 2.0f)); // take quarter step back to find accurate starting angle
                }
            }

            // Now we have starting angle, just spawn eyes and flails
            for (uint32 i = 0; i < EYES_IN_10_HEROIC; i++)
            {
                float spawnAngle = MapManager::NormalizeOrientation(MOST_PLAYERS_ANGLE + (M_PI / 4.0f * (float)i));
                float spawnOrientaton = MapManager::NormalizeOrientation(spawnAngle + M_PI);

                switch (i)
                {
                case 1:
                case 3:
                {
                    pos.m_positionX = MIDDLE_X + cos(spawnAngle) * FLAIL_SPAWN_RANGE;
                    pos.m_positionY = MIDDLE_Y + sin(spawnAngle) * FLAIL_SPAWN_RANGE;

                    float z = me->GetMap()->GetHeight2(pos.GetPositionX(), pos.GetPositionY(), me->GetPositionZ());

                    me->SummonCreature(FLAIL_OF_GORATH_ENTRY, pos.GetPositionX(), pos.GetPositionY(), z, spawnOrientaton);
                    // no break intended !!!
                }
                case 0:
                case 2:
                case 4:
                {
                    pos.m_positionX = MIDDLE_X + cos(spawnAngle) * EYE_SPAWN_RANGE;
                    pos.m_positionY = MIDDLE_Y + sin(spawnAngle) * EYE_SPAWN_RANGE;

                    float z = me->GetMap()->GetHeight2(pos.GetPositionX(), pos.GetPositionY(), me->GetPositionZ());
                    z = EYE_Z; // GetHeight2 not returning correct height, why ?

                    me->SummonCreature(EYE_OF_GORATH_ENTRY, pos.GetPositionX(), pos.GetPositionY(), z, spawnOrientaton);

                    break;
                }
                default:
                    break;
                }
            }

            // Summon Claws
            for (uint32 i = 0; i < CLAWS_IN_10_HEROIC; i++)
            {
                float x, y, z;
                me->GetNearPoint(me, x, y, z, 1.0f, frand(20.0f, 30.0f), MOST_PLAYERS_ANGLE);

                me->SummonCreature(CLAW_OF_GORATH_ENTRY, x, y, z);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            scheduler.Update(diff);

            if (phase == NORMAL_PHASE)
            {
                if (voidSphereTimer <= diff)
                {
                    if (IsCastingAllowed())
                    {
                        SpawnVoidSphere();
                        voidSphereTimer = MAX_TIMER;
                    }
                }
                else voidSphereTimer -= diff;

                if (psychicDrainTimer <= diff)
                {
                    if(IsCastingAllowed())
                    {
                        me->CastSpell(me->GetVictim(), SPELL_PSYCHIC_DRAIN, false);
                        psychicDrainTimer = urand(20000, 25000);
                    }
                }
                else psychicDrainTimer -= diff;

                if (focusedAngerTimer <= diff)
                {
                    me->CastSpell(me, SPELL_FOCUSED_ANGER, true);
                    focusedAngerTimer = 5000;
                }
                else focusedAngerTimer -= diff;

                if (disruptingShadowsTimer <= diff)
                {
                    uint32 max = Is25ManRaid() ? urand(7,10) : 3;
                    me->CastCustomSpell(SPELL_DISRUPTING_SHADOWS, SPELLVALUE_MAX_TARGETS, max, me, true); // triggers Zon'ozz Whisper: Shadows

                    quoteIndex = urand(0, MAX_DISRUPTING_QUOTES - 1);

                    RunPlayableQuote(disruptingQuotes[quoteIndex]);

                    disruptingShadowsTimer = 25000;
                }
                else disruptingShadowsTimer -= diff;
            }
            else if (phase == TRANSITION_PHASE)
            {
                // nothing so far -> handled in MovementInform
            }
            else if (phase == DARK_PHASE)
            {
                me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

                if (roarTimer <= diff)
                {
                    // scream periodicaly through whole dark phase + players camera should shake a bit
                    me->CastSpell(me, SPELL_ROAR_EMOTE, true);
                    me->CastSpell(me, SPELL_CAMERA_SHAKE, true);
                    roarTimer = 2000;
                }
                else roarTimer -= diff;
            }

            if (phase == NORMAL_PHASE)
                DoMeleeAttackIfReady();
        }
    };
};

// These spells are probably not for this encounter, but help us a lot (because originally it should be handled by spell 103521, but it is triggering unknown spell 103522)
#define CLUMP_CHECK_1 98399
#define CLUMP_CHECK_2 100943

class npc_warlord_void_sphere : public CreatureScript
{
public:
    npc_warlord_void_sphere() : CreatureScript("npc_warlord_void_sphere") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_warlord_void_sphereAI(creature);
    }

    struct npc_warlord_void_sphereAI : public ScriptedAI
    {
        npc_warlord_void_sphereAI(Creature* creature) : ScriptedAI(creature)
        {
            bossGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        uint64 bossGUID;

        // Timers and stuff
        uint32 moveTimer;
        uint32 checkTimer;
        uint32 refreshTimer;

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetSpeed(MOVE_RUN, 0.8f, true);
            moveTimer = 4000;
            checkTimer = 1000;
            refreshTimer = 4000;
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell) override
        {
            if (target->GetTypeId() != TYPEID_PLAYER)
                return;

            if (!me->HasAura(SPELL_VOID_OF_THE_UNMAKING_VISUAL))
                return;

            if (spell->Id == CLUMP_CHECK_1 || spell->Id == CLUMP_CHECK_2)
            {
                me->CastSpell(me, SPELL_VOID_DIFFUSION_DAMAGE, true);
                me->CastSpell(me, SPELL_VOID_OF_THE_UNMAKING_REMOVE, true);

                if (Creature * pBoss = ObjectAccessor::GetCreature(*me, bossGUID))
                    pBoss->AI()->DoAction(ACTION_SPHERE_BOUNCE);

                UpdateMovePoint(true);
                moveTimer = 2000;
                refreshTimer = 5000;
            }
        }

        void UpdateMovePoint(bool reverse = false)
        {
            float x, y, z;
            float angle = me->GetOrientation();

            if (reverse)
            {
                angle = MapManager::NormalizeOrientation(angle + M_PI); // Switch direction
                float angleDiff = M_PI / 20.0f; // Sphere should turn aside a bit
                angle = MapManager::NormalizeOrientation(urand(0,1) ? (angle + angleDiff) : (angle - angleDiff));
            }

            me->GetNearPoint(me, x, y, z, 10.0f, me->GetObjectSize(), angle);
            me->GetMotionMaster()->MovePoint(0, x, y, z, false, false);
        }

        void EnterCombat(Unit * /*who*/) override {}
        void MoveInLineOfSight(Unit * /*who*/) override {}
        void EnterEvadeMode() override {}

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            if (Creature * pBoss = ObjectAccessor::GetCreature(*me,bossGUID))
                pBoss->AI()->KilledUnit(victim);
        }

        void IsSummonedBy(Unit* pSummoner) override
        {
            if (pSummoner && pSummoner->ToCreature())
                bossGUID = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (refreshTimer <= diff)
            {
                // Collision is allowed only with this aura
                me->CastSpell(me, SPELL_VOID_OF_THE_UNMAKING_VISUAL, true);
                refreshTimer = MAX_TIMER;
            }
            else refreshTimer -= diff;

            if (checkTimer <= diff)
            {
                bool sphereHitBoss = false;

                if (Creature * pBoss = ObjectAccessor::GetCreature(*me, bossGUID))
                {
                    if (me->GetExactDist2d(pBoss) < 10.0f && me->HasAura(SPELL_VOID_OF_THE_UNMAKING_VISUAL))
                    {
                        sphereHitBoss = true;
                        pBoss->AI()->DoAction(ACTION_SPHERE_HIT_BOSS);
                        me->ForcedDespawn(100);
                        moveTimer = 10000;
                        refreshTimer = 10000;
                    }
                }

                // AoE dummy spell (8 yards)
                if (!sphereHitBoss)
                    me->CastSpell(me, CLUMP_CHECK_1, true);

                checkTimer = 1000;
            }
            else checkTimer -= diff;

            if (moveTimer <= diff)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                if (me->GetDistance2d(MIDDLE_X,MIDDLE_Y) >= ROOM_RADIUS)
                {
                    me->CastSpell(me, SPELL_BLACK_BLOOD_ERUPTION, true);
                    me->CastSpell(me, SPELL_VOID_DIFFUSION_STACK, true);
                    UpdateMovePoint(true);
                    moveTimer = 4000;
                    return;
                }
                else
                    UpdateMovePoint();

                moveTimer = 2000;
            }
            else moveTimer -= diff;
        }
    };
};

struct TentacleAI : public ScriptedAI
{
    TentacleAI(Creature* c) : ScriptedAI(c)
    {
        bossGUID = 0;
        abilityTimer = 10000;
    }

    public:
    uint64 bossGUID;
    uint32 abilityTimer;

    void Reset() override
    {
        if (!IsHeroic())
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }
        else
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        // Every tentacle should not be able to move
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
        // Add visual black blood auras
        ApplyBlackBloodAuras();

        if (IsHeroic() || me->GetEntry() == EYE_OF_GORATH_ENTRY)
            me->SetInCombatWithZone();
    }

    void IsSummonedBy(Unit* pSummoner) override
    {
        if (pSummoner->ToCreature())
            bossGUID = pSummoner->GetGUID();
    }

    void KilledUnit(Unit * victim) override
    {
        if (Creature * pZonozz = GetSummnoner())
            pZonozz->AI()->KilledUnit(victim);
    }

    void JustDied(Unit * killer) override
    {
        // Ignore normal difficulty
        if (!IsHeroic())
            return;

        Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();

        for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            if (Player* player = i->getSource())
            {
                // Drop stack of black blood if tentacle is killed
                if (Aura * aBlackBlood = player->GetAura(Is25ManRaid() ? SPELL_BLACK_BLOOD_25_HC : SPELL_BLACK_BLOOD_10_HC))
                {
                    aBlackBlood->ModStackAmount(-1);
                    aBlackBlood->RefreshDuration(); // no need to refresh, because infinite duration, but this will also cause to periodic effect to recalculate properly ...
                }
            }
    }

    void ApplyBlackBloodAuras()
    {
        switch (me->GetEntry())
        {
            case EYE_OF_GORATH_ENTRY:
                me->CastSpell(me, SPELL_BLACK_BLOOD_CIRCLE_EYE, true);
                break;
            case FLAIL_OF_GORATH_ENTRY:
                me->CastSpell(me, SPELL_BLACK_BLOOD_SPIT_FLAIL, true);
                break;
            case CLAW_OF_GORATH_ENTRY:
                me->CastSpell(me, SPELL_BLACK_BLOOD_SPIT_CLAW, true);
                break;
            default:
                break;
        }
    }

    bool CanTentacleMeleeAttack()
    {
        return IsHeroic() && me->GetEntry() != EYE_OF_GORATH_ENTRY;
    }

    Creature * GetSummnoner()
    {
        return ObjectAccessor::GetCreature(*me, bossGUID);
    }

    void UpdateAI(const uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (!CanTentacleMeleeAttack())
            return;

        if (me->GetEntry() == CLAW_OF_GORATH_ENTRY)
        {
            DoMeleeAttackIfReady();
        }
    }
};

class npc_eye_of_gorath : public CreatureScript
{
public:
    npc_eye_of_gorath() : CreatureScript("npc_eye_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_eye_of_gorathAI(creature);
    }

    struct npc_eye_of_gorathAI : public TentacleAI
    {
        npc_eye_of_gorathAI(Creature* creature) : TentacleAI(creature)
        {
            abilityTimer = urand(2000,4000);
        }

        const int32 SPELL_SHADOW_GAZE_BY_DIFFICULTY = ScriptedAI::RAID_MODE<int32>(104347, 104602, 104603, 104604);

        void UpdateAI(const uint32 diff) override
        {
            TentacleAI::UpdateAI(diff);

            if (abilityTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 300.0f, true, -SPELL_SHADOW_GAZE_BY_DIFFICULTY))
                        me->CastSpell(player, SPELL_SHADOW_GAZE, false);

                    abilityTimer = 3000;
                }
            }
            else abilityTimer -= diff;
        }
    };
};

class npc_claw_of_gorath : public CreatureScript
{
public:
    npc_claw_of_gorath() : CreatureScript("npc_claw_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_claw_of_gorathAI(creature);
    }

    struct npc_claw_of_gorathAI : public TentacleAI
    {
        npc_claw_of_gorathAI(Creature* creature) : TentacleAI(creature)
        {
            abilityTimer = 6000;
        }

        void UpdateAI(const uint32 diff) override
        {
            TentacleAI::UpdateAI(diff);

            if (!IsHeroic())
                return;

            if (abilityTimer <= diff)
            {
                // When not tanked the Claw of Go'rath spits ooze at a random player
                if (!me->IsWithinMeleeRange(me->GetVictim()))
                {
                    if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                        me->CastSpell(player, SPELL_OOZE_SPIT, false);
                }
                abilityTimer = 3000;
            }
            else abilityTimer -= diff;
        }
    };
};

class npc_flail_of_gorath : public CreatureScript
{
public:
    npc_flail_of_gorath() : CreatureScript("npc_flail_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_flail_of_gorathAI(creature);
    }

    struct npc_flail_of_gorathAI : public TentacleAI
    {
        npc_flail_of_gorathAI(Creature* creature) : TentacleAI(creature)
        {
            abilityTimer    = urand(4000,5000);
            sludgeSpewTimer = urand(2000, 4000);
        }

        uint32 sludgeSpewTimer;

        void UpdateAI(const uint32 diff) override
        {
            TentacleAI::UpdateAI(diff);

            if (!IsHeroic())
                return;

            if (abilityTimer <= diff)
            {
                me->CastSpell(me, SPELL_WILD_FLAIL, false);
                abilityTimer = 5000;
            }
            else abilityTimer -= diff;

            if (sludgeSpewTimer <= diff)
            {
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                    me->CastSpell(player, SPELL_SLUDGE_SPEW, false);
                sludgeSpewTimer = 2500;
            }
            else sludgeSpewTimer -= diff;
        }
    };
};

class spell_gen_disrupting_shadows : public SpellScriptLoader
{
public:
    spell_gen_disrupting_shadows() : SpellScriptLoader("spell_gen_disrupting_shadows") { }

    class spell_gen_disrupting_shadows_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_disrupting_shadows_AuraScript);

        void OnDispel(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_ENEMY_SPELL)
            {
                if (Unit* auraOwner = GetUnitOwner())
                    auraOwner->CastSpell(auraOwner, SPELL_DISRUPTING_SHADOWS_KNOCKBACK, true);
            }
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_disrupting_shadows_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const override
    {
        return new spell_gen_disrupting_shadows_AuraScript();
    }
};

void AddSC_boss_warlord_zonozz()
{
    new boss_warlord_zonozz();                  // 55308
    new npc_warlord_void_sphere();              // 55334
    new npc_eye_of_gorath();                    // 55416
    new npc_claw_of_gorath();                   // 55418
    new npc_flail_of_gorath();                  // 55417
    new spell_gen_disrupting_shadows();         // 103434, 104599, 104600, 104601
}

/* select * from creature_template where entry in (55308,55334,55416,55418,55417)

REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103434, 'spell_gen_disrupting_shadows');
REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104599, 'spell_gen_disrupting_shadows');
REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104600, 'spell_gen_disrupting_shadows');
REPLACE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104601, 'spell_gen_disrupting_shadows');

*/