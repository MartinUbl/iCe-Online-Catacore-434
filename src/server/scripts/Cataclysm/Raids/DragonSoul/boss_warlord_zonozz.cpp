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
#include "dragonsoul.h"
#include "MapManager.h"

enum spells
{
    /* BOSS SPELLS */
    SPELL_FOCUSED_ANGER                 = 104543, // OK
    SPELL_PSYCHIC_DRAIN                 = 104323, // OK
    SPELL_DISRUPTING_SHADOWS            = 103434, // OK (maybe cast with max affected targets)
    SPELL_DISRUPTING_SHADOWS_KNOCKBACK  = 103948, // this should be triggered after dispeling spell above
    SPELL_BLACK_BLOOD_INFINTY           = 104377, // infinite duration (in heroic ?)
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
    SPELL_BLACK_BLOOD_ERUPTION          = 110382, // if sphere hits the edge of the room -> TARGET_UNIT_CASTER -> cast by players themsleves probably

    /* SUMMONS SPELLS*/
    SPELL_OOZE_SPIT                     = 109396, // Casted by Claw of Go'rath when no one in melee range
    SPELL_SHADOW_GAZE                   = 104347, // Casted by Eye of Go'rath to random player
    SPELL_WILD_FLAIL                    = 109199, // Casted be Flail of Go'rath (melee AoE)
    SPELL_SLUDGE_SPEW                   = 110102, // Casted be Flail of Go'rath to random player
};

enum entries
{
    WARLORD_ENTRY               = 55308,
    CLAW_OF_GORATH_ENTRY        = 55418,
    EYE_OF_GORATH_ENTRY         = 57875,
    FLAIL_OF_GORATH_ENTRY       = 55417,
    VOID_SPHERE_ENTRY           = 55334
};

struct Quote
{
    uint32 soundId;
    const char * text;
};

// Once I will learn Faceless language, so cool :P

static const Quote introQuote = { 26337, "Vwyq agth sshoq'meg N'Zoth vra zz shfk qwor ga'halahs agthu. Uulg'ma, ag qam." };
static const Quote aggroQuote = { 26335, "Zzof Shuul'wah. Thoq fssh N'Zoth!" };
static const Quote blackQuote = { 26344, "N'Zoth ga zyqtahg iilth." };
static const Quote voidQuote =  { 26345, "Gul'kafh an'qov N'Zoth." };
static const Quote deathQuote = { 26336, "Uovssh thyzz... qwaz..." };

static const Quote disruptingQuotes[3] = 
{
    { 26340, "Sk'shgn eqnizz hoq." },
    { 26342, "Sk'magg yawifk hoq." },
    { 26343, "Sk'uuyat guulphg hoq." },
};

static const Quote killQuotes[3] = 
{
    { 26338, "Sk'tek agth nuq N'Zoth yyqzz." },
    { 26339, "Sk'shuul agth vorzz N'Zoth naggwa'fssh." },
    { 26341, "Sk'yahf agth huqth N'Zoth qornaus." },
};

#define MIDDLE_X (-1766.0f)
#define MIDDLE_Y (-1917.0f)
#define MIDDLE_Z (-226.00f)

#define MIDDLE_RADIUS (105.0f)

#define EYE_Z (-221.0f)


#define DARK_PHASE_NORMAL (30000)

const Position eyePos [8]  =
{
    {-1791.0f,-1989.0f,EYE_Z,1.51f},
    {-1733.0f,-1985.0f,EYE_Z,1.99f},
    {-1694.0f,-1940.0f,EYE_Z,2.95f},
    {-1701.0f,-1883.0f,EYE_Z,3.57f},
    {-1745.0f,-1845.0f,EYE_Z,4.50f},
    {-1802.0f,-1849.0f,EYE_Z,5.30f},
    {-1840.0f,-1894.0f,EYE_Z,6.03f},
    {-1835.0f,-1951.0f,EYE_Z,0.48f}
};

typedef uint8 PHASE;

enum phases
{
    NORMAL_PHASE,
    TRANSITION_PHASE,
    DARK_PHASE,
};

enum actions
{
    ACTION_SPHERE_HIT_BOSS,
};

class boss_warlord_zonozz : public CreatureScript
{
public:
    boss_warlord_zonozz() : CreatureScript("boss_warlord_zonozz") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_warlord_zonozzAI(creature);
    }

    struct boss_warlord_zonozzAI : public ScriptedAI
    {
        boss_warlord_zonozzAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            introDone = false;
        }

        InstanceScript* instance;
        uint32 voidSphereTimer;
        uint32 focusedAngerTimer;
        uint32 psychicDrainTimer;
        uint32 disruptingShadowsTimer;
        uint32 roarTimer;
        uint32 enrageTimer;
        bool introDone;
        SummonList summons;


        PHASE phase;
        uint32 phaseTimer;

        void Reset()
        {
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,10.0f);
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            summons.DespawnAll();
            phase = NORMAL_PHASE;
            me->RemoveAllAuras();
        }

        void JustSummoned(Creature *pSummon)
        {
            summons.Summon(pSummon);
        }

        void SummonedCreatureDespawn(Creature *pSummon)
        {
            switch (pSummon->GetEntry())
            {
                case EYE_OF_GORATH_ENTRY:
                {
                    phase = NORMAL_PHASE;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    focusedAngerTimer = 6000;
                    disruptingShadowsTimer = 6000;
                    if (me->GetVictim())
                    {
                        me->SetUInt64Value(UNIT_FIELD_TARGET, me->GetVictim()->GetGUID());
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                    }
                    break;
                }
            }
            summons.Despawn(pSummon);
        }

        void PlayQuote(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me,soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void ReceiveEmote(Player* pPlayer, uint32 text_emote) // This is only for testing purpose
        {
            phaseTimer = urand(1000, 2000); // Switch phase after void sphere despawn
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_SPHERE_HIT_BOSS)
            {
                phaseTimer = 500; // Switch phase after void sphere hit the boss
                // Remove all stacks of focused anger on boss
                me->RemoveAurasDueToSpell(SPELL_FOCUSED_ANGER);

                /*me->RemoveAura(109409);
                me->RemoveAura(109410);
                me->RemoveAura(109411);*/

                voidSphereTimer = DARK_PHASE_NORMAL + 28000; // Default time of next sphere
                psychicDrainTimer = voidSphereTimer + 8500;

                // Add 5% damage taken debuff for every bounce of the sphere
                if (Creature * pSphere = me->FindNearestCreature(VOID_SPHERE_ENTRY,200.0f,true))
                if (Aura * pAura = pSphere->GetAura(SPELL_VOID_DIFFUSION_STACK))
                {
                    uint32 stacks = pAura->GetStackAmount();
                    for (uint32 i = 0; i < stacks; i++)
                        me->AddAura(SPELL_VOID_DIFFUSION_TAKEN, me);

                    stacks = stacks > 7 ? 7 : stacks;

                    voidSphereTimer = DARK_PHASE_NORMAL + (DARK_PHASE_NORMAL - (stacks * IN_MILLISECONDS * 4)); // 4 seconds reduction for every bounce
                    psychicDrainTimer = voidSphereTimer + 8500;
                }
            }
        }

        void EnterCombat(Unit * /*who*/)
        {
            PlayQuote(aggroQuote.soundId, aggroQuote.text);
            if (instance)
                me->MonsterYell("INSTANCE OK",0,0);

            enrageTimer = 6 * MINUTE * IN_MILLISECONDS;
            phaseTimer = MAX_TIMER; // must be set before summonig of sphere
            roarTimer = MAX_TIMER;
            voidSphereTimer = MAX_TIMER;
            focusedAngerTimer = 10500;
            psychicDrainTimer = 13000;

            me->SetInCombatWithZone();
            SpawnVoidSphere();
        }

        void SpawnVoidSphere()
        {
            //me->CastSpell(me, SPELL_VOID_OF_THE_UNMAKING_SUMMON, true);

            PlayQuote(voidQuote.soundId, voidQuote.text);

            // Summon void sphere inf frontof the boss and cast beam on it
            float x, y, z;
            me->GetNearPoint(me, x, y, z, 30.0f, 0.0f, me->GetOrientation());

            if (Creature * sphere = me->SummonCreature(VOID_SPHERE_ENTRY, x, y, z, me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                me->CastSpell(sphere, SPELL_VOID_OF_THE_UNMAKING_CHANNEL, false);
        }

        void EnterEvadeMode()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 randPos = urand(0, 2);
                PlayQuote(killQuotes[randPos].soundId, killQuotes[randPos].text);
            }
        }

        void JustDied(Unit* /*who*/)
        {
            PlayQuote(deathQuote.soundId, deathQuote.text);
            summons.DespawnAll();
        }

        void MoveInLineOfSight(Unit *who)
        {
            if (!introDone && who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && who->GetExactDist2d(me) < 60.0f)
            {
                PlayQuote(introQuote.soundId, introQuote.text);
                introDone = true;
                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            }
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;
            
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, SPELL_DARKNESS, true);
            me->CastSpell(me, SPELL_BLACK_BLOOD, true);

            uint32 rands[] = {0,1,2,3,4,5,6,7};
            std::vector<uint32> randInts (rands, rands + sizeof(rands) / sizeof(uint32));
            std::random_shuffle (randInts.begin(),randInts.end());

            // Summon 5 eyes in 10 man -> 8 eyes in 25 man
            uint32 maxSummons = Is25ManRaid() ? 8 : 5;

            for (uint32 i = 0; i < maxSummons; i++)
                me->SummonCreature(EYE_OF_GORATH_ENTRY, eyePos[i], TEMPSUMMON_TIMED_DESPAWN, 28000);

            //Summon one claw
            me->SummonCreature(CLAW_OF_GORATH_ENTRY, GetRandomPositionInRadius(20.0f,30.0f), TEMPSUMMON_TIMED_DESPAWN, 28000);
            //Summon one flail
            me->SummonCreature(FLAIL_OF_GORATH_ENTRY, GetRandomPositionInRadius(40.0f,50.0f), TEMPSUMMON_TIMED_DESPAWN, 28000);
            // Can enter to dark phase
            phase = DARK_PHASE;
            //voidSphereTimer = 30000 + 18000;
            psychicDrainTimer = DARK_PHASE_NORMAL + 18000 + 10000;
            roarTimer = 500;

            PlayQuote(blackQuote.soundId, blackQuote.text);
        }

        Position GetRandomPositionInRadius(float min_radius,float max_radius)
        {
            float radius = frand(min_radius, max_radius);
            float angle = frand(0.0f,2*M_PI);
            Position pos;
            pos.m_positionX = MIDDLE_X + cos(angle)*radius;
            pos.m_positionY = MIDDLE_Y + sin(angle)*radius;
            pos.m_positionZ = me->GetBaseMap()->GetHeight(me->GetPhaseMask(),MIDDLE_X,MIDDLE_Y,MIDDLE_Z,true) + 1.0f;
            return pos;
        }

        bool CanCast()
        {
            if (me->IsNonMeleeSpellCasted(false))
                return false;
            else
                return true;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (enrageTimer <= diff)
            {
                me->CastSpell(me, SPELL_BERSERK, true);
                enrageTimer = MAX_TIMER;
            }
            else enrageTimer -= diff;

            if (phaseTimer <= diff)
            {
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->MovePoint(0, MIDDLE_X, MIDDLE_Y, MIDDLE_Z);
                phase = TRANSITION_PHASE;
                phaseTimer = MAX_TIMER; // not happend periodicaly
                return;
            }
            else phaseTimer -= diff;

            if (phase == NORMAL_PHASE)
            {
                if (voidSphereTimer <= diff)
                {
                    if (CanCast())
                    {
                        SpawnVoidSphere();
                        voidSphereTimer = MAX_TIMER;
                    }
                }
                else voidSphereTimer -= diff;

                if (psychicDrainTimer <= diff)
                {
                    if(CanCast())
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
                    me->CastCustomSpell(SPELL_DISRUPTING_SHADOWS, SPELLVALUE_MAX_TARGETS,max, me, true);

                    uint32 randPos = urand(0, 2);
                    PlayQuote(disruptingQuotes[randPos].soundId, disruptingQuotes[randPos].text);

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

// These spells are probably not for this encounter, but help us a lot
#define CLUMP_CHECK_1 98399
#define CLUMP_CHECK_2 100943 

class npc_warlord_void_sphere : public CreatureScript
{
public:
    npc_warlord_void_sphere() : CreatureScript("npc_warlord_void_sphere") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_warlord_void_sphereAI(creature);
    }

    struct npc_warlord_void_sphereAI : public ScriptedAI
    {
        npc_warlord_void_sphereAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            bossGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        InstanceScript* instance;
        uint64 bossGUID;
        // Timers and stuff
        uint32 moveTimer;
        uint32 checkTimer;
        uint32 refreshTimer;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetSpeed(MOVE_RUN, 0.8f, true);
            moveTimer = 4000;
            checkTimer = 1000;
            refreshTimer = 4000;
        }

        void SpellHitTarget(Unit* target, const SpellEntry* spell)
        {
            if (target->GetTypeId() != TYPEID_PLAYER)
                return;

            if (!me->HasAura(SPELL_VOID_OF_THE_UNMAKING_VISUAL))
                return;

            if (spell->Id == CLUMP_CHECK_1 || spell->Id == CLUMP_CHECK_2)
            {
                me->CastSpell(me, SPELL_VOID_DIFFUSION_DAMAGE, true);
                me->CastSpell(me, SPELL_VOID_DIFFUSION_STACK, true);
                me->CastSpell(me, SPELL_VOID_OF_THE_UNMAKING_REMOVE, true);
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

        void CastEruptionByPlayers()
        {
            Map::PlayerList const &PlayerList = me->GetMap()->GetPlayers();

            if (!PlayerList.isEmpty())
                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                if (Player* pPlayer = i->getSource())
                    pPlayer->CastSpell(pPlayer, SPELL_BLACK_BLOOD_ERUPTION, true,NULL,NULL,me->GetGUID());
        }

        void EnterCombat(Unit * /*who*/) {}
        void MoveInLineOfSight(Unit * /*who*/) {}
        void EnterEvadeMode() {}

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;
            if (Creature * pBoss = ObjectAccessor::GetCreature(*me,bossGUID))
                pBoss->AI()->KilledUnit(victim);
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                bossGUID = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff)
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
                // AoE dummy spell (8 yards)
                me->CastSpell(me, CLUMP_CHECK_1, true);

                if (Creature * pBoss = me->FindNearestCreature(WARLORD_ENTRY,50.0f,true))
                {
                    if (me->GetExactDist2d(pBoss) < 10.0f && !pBoss->HasAura(SPELL_DARKNESS))
                    {
                        pBoss->AI()->DoAction(ACTION_SPHERE_HIT_BOSS);
                        me->ForcedDespawn(200);
                    }
                }
                checkTimer = 1000;
            }
            else checkTimer -= diff;

            if (moveTimer <= diff)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                if (me->GetDistance2d(MIDDLE_X,MIDDLE_Y) >= MIDDLE_RADIUS)
                {
                    CastEruptionByPlayers();
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

class npc_eye_of_gorath : public CreatureScript
{
public:
    npc_eye_of_gorath() : CreatureScript("npc_eye_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_of_gorathAI(creature);
    }

    struct npc_eye_of_gorathAI : public ScriptedAI
    {
        npc_eye_of_gorathAI(Creature* creature) : ScriptedAI(creature) { bossGUID = 0; }

        uint32 beamTimer;
        uint64 bossGUID;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetInCombatWithZone();
            beamTimer = urand(1000, 4000);
        }

        void DoAction(const int32 action)
        {
        }

        void EnterCombat(Unit * /*who*/)
        {
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;
            if (Creature * pBoss = ObjectAccessor::GetCreature(*me,bossGUID))
                pBoss->AI()->KilledUnit(victim);
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                bossGUID = pSummoner->GetGUID();
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (beamTimer <= diff)
            {
                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true,-SPELL_SHADOW_GAZE))
                    me->CastSpell(target, SPELL_SHADOW_GAZE, true);
                beamTimer = 3000;
            }
            else beamTimer -= diff;
        }
    };
};

class npc_claw_of_gorath : public CreatureScript
{
public:
    npc_claw_of_gorath() : CreatureScript("npc_claw_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_claw_of_gorathAI(creature);
    }

    struct npc_claw_of_gorathAI : public ScriptedAI
    {
        npc_claw_of_gorathAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            bossGUID = 0;
        }

        InstanceScript* instance;
        uint64 bossGUID;
        // Timers and stuff

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void DoAction(const int32 action)
        {
        }

        void EnterCombat(Unit * /*who*/)
        {
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;
            if (Creature * pBoss = ObjectAccessor::GetCreature(*me,bossGUID))
                pBoss->AI()->KilledUnit(victim);
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                bossGUID = pSummoner->GetGUID();
        }

        void JustDied(Unit* /*killer*/)
        {
        }

        void UpdateAI(const uint32 diff)
        {
        }
    };
};

class npc_flail_of_gorath : public CreatureScript
{
public:
    npc_flail_of_gorath() : CreatureScript("npc_flail_of_gorath") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_flail_of_gorathAI(creature);
    }

    struct npc_flail_of_gorathAI : public ScriptedAI
    {
        npc_flail_of_gorathAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            bossGUID = 0;
        }

        InstanceScript* instance;
        uint64 bossGUID;
        // Timers and stuff

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void DoAction(const int32 action)
        {
        }

        void EnterCombat(Unit * /*who*/)
        {
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;
            if (Creature * pBoss = ObjectAccessor::GetCreature(*me,bossGUID))
                pBoss->AI()->KilledUnit(victim);
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                bossGUID = pSummoner->GetGUID();
        }

        void JustDied(Unit* /*killer*/)
        {
        }

        void UpdateAI(const uint32 diff)
        {
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

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_disrupting_shadows_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_disrupting_shadows_AuraScript();
    }
};


void AddSC_boss_warlord_zonozz()
{
    new boss_warlord_zonozz(); // 55308
    new npc_warlord_void_sphere(); // 55334
    new npc_eye_of_gorath(); // 57875
    new npc_claw_of_gorath(); // 55418
    new npc_flail_of_gorath(); // 55417
    new spell_gen_disrupting_shadows(); // 103434,104599,104600,104601
}

/* select * from creature_template where entry in (55308,55334,57875,55418,55417)

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103434, 'spell_gen_disrupting_shadows');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104599, 'spell_gen_disrupting_shadows');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104600, 'spell_gen_disrupting_shadows');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104601, 'spell_gen_disrupting_shadows');


*/