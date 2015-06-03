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


#define MIDDLE_X (-1763.7f)
#define MIDDLE_Y (-3032.5f)
#define MIDDLE_Z (-182.40f)

#define GLOBULE_Z (-172.0f)

struct FacelessQuote
{
    uint32 soundId;
    const char * facelessText;
    const char * whisperText;
};

static const FacelessQuote introQuote =
{
    26328,
    "Ak'agthshi ma uhnish, ak'uq shg'cul vwahuhn! H'iwn iggksh Phquathi gag OOU KAAXTH SHUUL!",
    "Our numbers are endless, our power beyond reckoning! All who oppose the Destroyer will DIE A THOUSAND DEATHS!"
};

static const FacelessQuote aggroQuote =
{   26326,
    "Iilth qi'uothk shn'ma yeh'glu Shath'Yar! H'IWN IILTH!",
    "You will drown in the blood of the Old Gods! ALL OF YOU!"
};

static const FacelessQuote deathQuote =
{   26327,
    "Ez, Shuul'wah! Sk'woth'gl yu'gaz yog'ghyl ilfah!",
    "O, Deathwing! Your faithful servant has failed you!"
};

static const FacelessQuote bloodQuotes[2] = 
{
    { 26332, "KYTH ag'xig yyg'far IIQAATH ONGG!", "SEE how we pour from the CURSED EARTH!" },
    { 26333, "UULL lwhuk H'IWN!", "The DARKNESS devours ALL!" },
};

static const FacelessQuote killQuotes[3] = 
{
    { 26329, "Sk'yahf qi'plahf PH'MAGG!", "Your soul will know ENDLESS TORMENT!" },
    { 26330, "H'iwn zaix Shuul'wah, PHQUATHI!", "All praise Deathwing, THE DESTROYER!" },
    { 26331, "Shkul an'zig qvsakf KSSH'GA, ag'THYZAK agthu!", "From its BLEAKEST DEPTHS, we RECLAIM this world!" },
};

enum spells
{
    SPELL_VOID_BOLT = 104849,
    SPELL_VOID_BOLT_AOE = 105416,
    SPELL_CHANNEL_ANIM_KIT  = 89176, // This guy channeling something, we dont know what shit is it ... use anim kit spell as fake channel
    SPELL_ENRAGE    = 26662,

    SPELL_BLACK_BLOOD_OF_SHUMA = 104894,
    SPELL_SHADOWED_BLOOD_OF_SHUMA = 104896,
    SPELL_CRIMSON_BLOOD_OF_SHUMA = 104897,
    SPELL_ACIDIC_BLOOD_OF_SHUMA = 104898,
    SPELL_COBALT_BLOOD_OF_SHUMA = 104900,
    SPELL_GLOWING_BLOOD_OF_SHUMA = 104901, // also increase attack speed by 50 %
    SPELL_FUSING_VAPOR              = 103635, // (TARGET_UNIT_SRC_AREA_ENTRY) this shoud hit other active globules, heal them a bit and make them immune to damage
    // Green phase
    SPELL_DIGESTIVE_ACID_VISUAL = 105562,
    SPELL_DIGESTIVE_ACID_MISSILE = 105573,
    // Dark phase
    SPELL_DARK_POOL_VISUAL      = 105600,
    SPELL_PSYCHIC_SLICE         =  105671,
    // Purple phase
    SPELL_DEEP_CORRUPTION_PROC_AURA = 105171,
    SPELL_DEEP_CORRUPTION_DAMAGE    = 105173,
    // Red phase
    SPELL_SEARING_BLOOD = 105033, // AoE
    // Blue phase
    SPELL_MANA_VOID_VISUAL_BUBBLE = 105527,
    SPELL_MANA_DIFFUSION_EXPLOSION = 105539, // energizing AoE to 100 % mana -> now it is only 100 mana
    SPELL_MANA_VOID_VISUAL_CASTING = 105505, // 3s dummy aura
    SPELL_MANA_VOID_DRAIN          = 105530, // aoe drain mana
};

enum entries
{
     YORSAH_ENTRY = 55312,
     MANA_VOID_ENTRY = 56231,
     FORGOTTEN_ONE_ENTRY = 56265,
};

enum colorBeamsAuras
{
    AURA_TALL_BLUE = 105473,
    AURA_TALL_RED = 105474,
    AURA_TALL_GREEN = 105475,
    AURA_TALL_YELLOW = 105476,
    AURA_TALL_PURPLE = 105477,
    AURA_TALL_BLACK = 105478,
};

enum colorCombinationsNormal // sniffed from DBM
{
    SPELL_COMB_PURPLE_GREEN_BLUE   = 105420,
    SPELL_COMB_GREEN_RED_BLACK     = 105435,
    SPELL_COMB_GREEN_YELLOW_RED    = 105436,
    SPELL_COMB_PURPLE_BLUE_YELLOW  = 105437,
    SPELL_COMB_BLUE_BLACK_YELLLOW  = 105439,
    SPELL_COMB_PURPLE_RED_BLACK    = 105440,
};

enum globuleEntry
{
    CRIMSON_GLOBULE_ENTRY = 55865, // red
    COBALT_GLOBULE_ENTRY = 55866, // blue
    GLOWING_GLOBULE_ENTRY = 55864, // yellow
    DARK_GLOBULE_ENTRY  = 55867, // black
    ACID_GLOBULE_ENTRY  = 55862, // green
    SHADOWED_GLOBULE_ENTRY = 55863 // purple
};

struct GlobuleData
{
    Position pos;
    uint32 globuleEntry;
};

const GlobuleData greenGlobePos  = {{-1721.0f,-2928.0f,GLOBULE_Z,4.27f},ACID_GLOBULE_ENTRY};
const GlobuleData blueGlobePos   = {{-1721.0f,-3140.0f,GLOBULE_Z,2.00f},COBALT_GLOBULE_ENTRY};
const GlobuleData purpleGlobePos = {{-1660.0f,-3078.0f,GLOBULE_Z,2.70f},SHADOWED_GLOBULE_ENTRY};
const GlobuleData yellowGlobePos = {{-1870.0f,-2990.0f,GLOBULE_Z,5.87f},GLOWING_GLOBULE_ENTRY};
const GlobuleData blackGlobePos  = {{-1809.0f,-3141.0f,GLOBULE_Z,1.15f},DARK_GLOBULE_ENTRY};
const GlobuleData redGlobePos    = {{-1659.0f,-2990.0f,GLOBULE_Z,3.55f},CRIMSON_GLOBULE_ENTRY};

struct spellCombination
{
    const uint32 spellId;
    const GlobuleData globules[3];
};

spellCombination spellCombs[6] =
{
    {   SPELL_COMB_PURPLE_GREEN_BLUE,   {purpleGlobePos,greenGlobePos,blueGlobePos}},
    {   SPELL_COMB_GREEN_RED_BLACK,     {greenGlobePos,redGlobePos,blackGlobePos}},
    {   SPELL_COMB_GREEN_YELLOW_RED,    {greenGlobePos,yellowGlobePos,redGlobePos}},
    {   SPELL_COMB_PURPLE_BLUE_YELLOW,  {purpleGlobePos,blueGlobePos,yellowGlobePos}},
    {   SPELL_COMB_BLUE_BLACK_YELLLOW,  {blueGlobePos,blackGlobePos,yellowGlobePos}},
    {   SPELL_COMB_PURPLE_RED_BLACK,    {purpleGlobePos,redGlobePos,blackGlobePos}},
};

#define MAX_COMBINATIONS        6
#define MAX_GLOBULES_NORMAL     3

static void PlayQuoteAndWhisper(Unit* source, uint32 soundId, const char * text, const char * whisperText)
{
    source->MonsterYell(text, LANG_UNIVERSAL, 0);
    source->PlayDirectSound(soundId);

    Map::PlayerList const &playerList = source->GetMap()->GetPlayers();

    if (!playerList.isEmpty())
        for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            if (Player* pPlayer = i->getSource())
                source->MonsterWhisper(whisperText, pPlayer->GetGUID());
}

class boss_yorsahj_the_unsleeping : public CreatureScript
{
public:
    boss_yorsahj_the_unsleeping() : CreatureScript("boss_yorsahj_the_unsleeping") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_yorsahj_the_unsleepingAI(creature);
    }

    struct boss_yorsahj_the_unsleepingAI : public ScriptedAI
    {
        boss_yorsahj_the_unsleepingAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            instance = creature->GetInstanceScript();
            introDone = false;
        }

        InstanceScript* instance;
        SummonList summons;
        bool introDone;
        uint32 forgottenCounter;
        uint32 abilityMask;

        // Timers and stuff
        uint32 oozeTimer;
        uint32 abilityTimer;
        uint32 attackTimer;
        uint32 manaVoidTimer;
        uint32 forgottenTimer;
        uint32 enrageTimer;

        void Reset()
        {
            forgottenCounter = 0;
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->RemoveAura(SPELL_CHANNEL_ANIM_KIT);
            summons.DespawnAll();
            oozeTimer = 22000;
            abilityTimer = 6000;
            attackTimer = MAX_TIMER;
            manaVoidTimer = MAX_TIMER;
            forgottenTimer = MAX_TIMER;
            enrageTimer = 10 * MINUTE * IN_MILLISECONDS;
        }

        void JustSummoned(Creature *pSummon)
        {
            summons.Summon(pSummon);
        }

        void SummonedCreatureDespawn(Creature *pSummon)
        {
            summons.Despawn(pSummon);
        }

        void MoveInLineOfSight(Unit *who)
        {
            if (!introDone && who->GetTypeId() == TYPEID_PLAYER && !who->ToPlayer()->IsGameMaster() && who->GetExactDist2d(me) < 60.0f)
            {
                PlayQuoteAndWhisper(me,introQuote.soundId, introQuote.facelessText,introQuote.whisperText);
                introDone = true;
                me->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            }
        }

        void SpawnManaVoid()
        {
            float x, y, z;
            me->GetNearPoint(me, x, y, z, 0.0f, 12.0f, frand(0, M_PI * 2.0f));
            z += 20;
            me->SummonCreature(MANA_VOID_ENTRY, x, y, z, 0.0f);
        }

        void DoAction(const int32 auraId)
        {
            attackTimer = 2000; // Stop channeling and attack again in 3 seconds
            abilityTimer = 4000;

            switch (auraId)
            {
                case SPELL_COBALT_BLOOD_OF_SHUMA:
                    me->CastSpell(me, SPELL_MANA_VOID_DRAIN, true);
                    me->CastSpell(me, SPELL_MANA_VOID_VISUAL_CASTING, true);
                    manaVoidTimer = 6000;
                    break;
                case SPELL_SHADOWED_BLOOD_OF_SHUMA:
                    break;
                case SPELL_CRIMSON_BLOOD_OF_SHUMA:
                    break;
                case SPELL_ACIDIC_BLOOD_OF_SHUMA:
                    me->CastSpell(me,SPELL_DIGESTIVE_ACID_VISUAL,true);
                    break;
                case SPELL_GLOWING_BLOOD_OF_SHUMA:
                    break;
                case SPELL_BLACK_BLOOD_OF_SHUMA:
                    forgottenTimer = 6000;
                    me->CastSpell(me,SPELL_DARK_POOL_VISUAL,true);
                    break;
            }
        }

        void EnterCombat(Unit * /*who*/)
        {
            PlayQuoteAndWhisper(me, aggroQuote.soundId, aggroQuote.facelessText, aggroQuote.whisperText);
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            uint32 randPos = urand(0, 2);
            PlayQuoteAndWhisper(me, killQuotes[randPos].soundId, killQuotes[randPos].facelessText, killQuotes[randPos].whisperText);
        }

        void JustDied()
        {
            summons.DespawnAll();
            PlayQuoteAndWhisper(me, deathQuote.soundId, deathQuote.facelessText, deathQuote.whisperText);
        }

        bool IsCasting()
        {
            if (me->IsNonMeleeSpellCasted(false) || me->HasAura(SPELL_CHANNEL_ANIM_KIT))
                return true;
            else
                return false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->HasAura(SPELL_CHANNEL_ANIM_KIT))
                me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

            if (enrageTimer <= diff)
            {
                me->CastSpell(me, SPELL_ENRAGE, true);
                enrageTimer = MAX_TIMER;
            }
            else enrageTimer -= diff;

            if (attackTimer <= diff)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveAura(SPELL_CHANNEL_ANIM_KIT);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                if (me->GetVictim())
                    me->SetUInt64Value(UNIT_FIELD_TARGET, me->GetVictim()->GetGUID());

                attackTimer = MAX_TIMER;

                if (me->HasAura(SPELL_COBALT_BLOOD_OF_SHUMA))
                    me->CastSpell(me, SPELL_MANA_VOID_VISUAL_CASTING, true);

                if (me->HasAura(SPELL_SHADOWED_BLOOD_OF_SHUMA))
                    me->CastSpell(me, SPELL_DEEP_CORRUPTION_PROC_AURA, true);
            }
            else attackTimer -= diff;

            if (manaVoidTimer <= diff)
            {
                SpawnManaVoid();
                manaVoidTimer = MAX_TIMER;
            }
            else manaVoidTimer -= diff;

            if (forgottenTimer <= diff)
            {
                uint32 max = Is25ManRaid() ? 10 : 5;
                if(forgottenCounter < max)
                {
                    float x, y, z;
                    me->GetNearPoint(me, x, y, z, 1, 12.0f, frand(0,2*M_PI));
                    me->SummonCreature(FORGOTTEN_ONE_ENTRY, x, y, z, MapManager::NormalizeOrientation(me->GetAngle(x, y) + M_PI));
                    forgottenCounter++;
                    forgottenTimer = 1000;
                }
                else
                    forgottenTimer = MAX_TIMER;
            }
            else forgottenTimer -= diff;

            if (abilityTimer <= diff)
            {
                if (!IsCasting() && me->GetVictim())
                {
                    abilityTimer = 6000;

                    if (me->HasAura(SPELL_GLOWING_BLOOD_OF_SHUMA))
                    {
                        me->CastSpell(me, SPELL_VOID_BOLT_AOE, true);
                        abilityTimer /= 2;
                    }
                    else
                        me->CastSpell(me->GetVictim(), SPELL_VOID_BOLT, true);

                    if (me->HasAura(SPELL_CRIMSON_BLOOD_OF_SHUMA))
                        me->CastCustomSpell(SPELL_SEARING_BLOOD, SPELLVALUE_MAX_TARGETS, Is25ManRaid() ? 8 : 3, me, true);

                    if (me->HasAura(SPELL_ACIDIC_BLOOD_OF_SHUMA))
                        me->CastSpell(me, SPELL_DIGESTIVE_ACID_MISSILE, true);
                }
            }
            else abilityTimer -= diff;

            if (oozeTimer <= diff)
            {
                if (!IsCasting())
                {
                    me->SetReactState(REACT_AGGRESSIVE);

                    uint32 randQuote = urand(0, 2);
                    PlayQuoteAndWhisper(me, bloodQuotes[randQuote].soundId, bloodQuotes[randQuote].facelessText, bloodQuotes[randQuote].whisperText);

                    // Pick random color combination
                    uint32 randColor = urand(0, MAX_COMBINATIONS - 1);

                    // Spawn globules by color combination
                    for (uint32 i = 0; i < MAX_GLOBULES_NORMAL; i++)
                    {
                        uint32 entry = spellCombs[randColor].globules[i].globuleEntry;
                        Position pos = spellCombs[randColor].globules[i].pos;
                        me->SummonCreature(entry,pos,TEMPSUMMON_MANUAL_DESPAWN,0);
                    }
                    // Cast appropriate combination spell
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                    me->CastSpell(me, spellCombs[randColor].spellId, false);
                    me->CastSpell(me, SPELL_CHANNEL_ANIM_KIT, true);
                    oozeTimer = 90000;
                }
            }
            else oozeTimer -= diff;

            if (!IsCasting())
                DoMeleeAttackIfReady();
        }
    };
};

enum animKit
{
    ANIM_KIT_EMERGE             = 1490,
};

enum globuleAuras
{
    BLACK_GLOBULE_AURA = 110746,
    COBALT_GLOBULE_AURA = 110747,
    SHADOWED_GLOBULE_AURA = 110748,
    CRIMSON_GLOBULE_AURA = 110750,
    GLOWING_GLOBULE_AURA = 110753,
    ACIDIC_GLOBULE_AURA = 110743,
};

enum globuleSpells
{
    SPELL_FUSING_VAPORS_PROC_AURA = 108235,
    SPELL_FUSING_VAPORS = 108233,
};

class npc_yorsahj_blood_globule : public CreatureScript
{
public:
    npc_yorsahj_blood_globule() : CreatureScript("npc_yorsahj_blood_globule") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_yorsahj_blood_globuleAI(creature);
    }

    struct npc_yorsahj_blood_globuleAI : public ScriptedAI
    {
        npc_yorsahj_blood_globuleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            empowerAuraId = beamAuraId = selfAuraId = 0;
            InitGlobuleSpells();
            vaporized = false;
        }

        InstanceScript* instance;
        uint32 empowerAuraId;
        uint32 beamAuraId;
        uint32 selfAuraId;
        bool vaporized;

        // Timers and stuff
        uint32 moveTimer;

        void Reset()
        {
            me->CastSpell(me, SPELL_FUSING_VAPORS_PROC_AURA, true);
            me->PlayOneShotAnimKit(ANIM_KIT_EMERGE);
            me->CastSpell(me, beamAuraId, true);
            me->CastSpell(me, selfAuraId, true);
            moveTimer = 7000;
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE); // TODO : SetFlag
            me->SetSpeed(MOVE_RUN, 0.7f, true);
        }

        void FuseVapor()
        {
            if (vaporized == false && me->GetHealthPct() < 50)
            {
                vaporized = true;

                std::list<Creature*> crList;
                for (uint32 entry = ACID_GLOBULE_ENTRY; entry <= DARK_GLOBULE_ENTRY; entry++)
                {
                    if (entry == me->GetEntry())
                        continue;

                    GetCreatureListWithEntryInGrid(crList, me, entry, 250.0f);
                    for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
                        (*itr)->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                }
            }
        }

        void UpdateMovePoint()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);

            Creature * pBoss = me->FindNearestCreature(YORSAH_ENTRY, 200.0f, true);
            if (pBoss == NULL)
                return;

            me->GetMotionMaster()->MovePoint(0, pBoss->GetPositionX(), pBoss->GetPositionY(), pBoss->GetPositionZ(), false, false);
        }

        bool BossReached()
        {
            Creature * pBoss = me->FindNearestCreature(YORSAH_ENTRY, 200.0f, true);
            if (pBoss == NULL)
                return false;

            return (me->GetExactDist2d(pBoss) <= 3.0f) ? true : false;
        }

        void InitGlobuleSpells()
        {
            switch (me->GetEntry())
            {
                case CRIMSON_GLOBULE_ENTRY:
                    selfAuraId = BLACK_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_RED;
                    empowerAuraId = SPELL_CRIMSON_BLOOD_OF_SHUMA;
                    break;
                case COBALT_GLOBULE_ENTRY:
                    selfAuraId = COBALT_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_BLUE;
                    empowerAuraId = SPELL_COBALT_BLOOD_OF_SHUMA;
                    break;
                case GLOWING_GLOBULE_ENTRY:
                    selfAuraId = GLOWING_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_YELLOW;
                    empowerAuraId = SPELL_GLOWING_BLOOD_OF_SHUMA;
                    break;
                case DARK_GLOBULE_ENTRY:
                    selfAuraId = BLACK_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_BLACK;
                    empowerAuraId = SPELL_BLACK_BLOOD_OF_SHUMA;
                    break;
                case ACID_GLOBULE_ENTRY:
                    selfAuraId = ACIDIC_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_GREEN;
                    empowerAuraId = SPELL_ACIDIC_BLOOD_OF_SHUMA;
                    break;
                case SHADOWED_GLOBULE_ENTRY:
                    selfAuraId = SHADOWED_GLOBULE_AURA;
                    beamAuraId = AURA_TALL_PURPLE;
                    empowerAuraId = SPELL_SHADOWED_BLOOD_OF_SHUMA;
                    break;
            }
        }

        void EnterCombat(Unit * /*who*/) {}
        void MoveInLineOfSight(Unit * /*who*/) {}
        void EnterEvadeMode() {}

        void UpdateAI(const uint32 diff)
        {
            if (moveTimer <= diff)
            {
                // Move to current boss position
                UpdateMovePoint();
                FuseVapor();
                // Check if we arrived to his position
                if (BossReached())
                {
                    if (Creature * pBoss = me->FindNearestCreature(YORSAH_ENTRY, 200.0f, true))
                    {
                        pBoss->AddAura(empowerAuraId, pBoss);
                        pBoss->AI()->DoAction(empowerAuraId);
                    }
                    me->ForcedDespawn(100);
                }
                moveTimer = 500;
            }
            else moveTimer -= diff;

        }
    };
};

class npc_mana_void_DS : public CreatureScript
{
public:
    npc_mana_void_DS() : CreatureScript("npc_mana_void_DS") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mana_void_DSAI(creature);
    }

    struct npc_mana_void_DSAI : public ScriptedAI
    {
        npc_mana_void_DSAI(Creature* creature) : ScriptedAI(creature) { exploded = false; }

        uint32 moveTimer;
        bool exploded;

        void Reset()
        {
            me->CastSpell(me, SPELL_MANA_VOID_VISUAL_BUBBLE, true);
            me->GetMotionMaster()->MoveFall();
            me->SetSpeed(MOVE_RUN, 0.7f, true);
            moveTimer = 3000;
        }

        void EnterCombat(Unit * /*who*/) {}
        void MoveInLineOfSight(Unit * /*who*/) {}
        void EnterEvadeMode() {}


        void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage)
        {
            if (exploded)
            {
                damage = 0;
                return;
            }

            if (damage > me->GetHealth())
            {
                exploded = true;
                damage = 0;
                me->StopMoving();
                me->CastSpell(me, SPELL_MANA_DIFFUSION_EXPLOSION, true);
                me->ForcedDespawn(1500);
                moveTimer = 2000;
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (moveTimer <= diff)
            {
                float x, y, z;
                me->GetNearPoint(me, x, y, z, 1, 10.0f, frand(0, 2 * M_PI));
                me->GetMotionMaster()->MovePoint(0, x, y, z, false, false);
                moveTimer = 4000;
            }
            else moveTimer -= diff;
        }
    };
};

class npc_forgotten_one_DS : public CreatureScript
{
public:
    npc_forgotten_one_DS() : CreatureScript("npc_forgotten_one_DS") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_forgotten_one_DSAI(creature);
    }

    struct npc_forgotten_one_DSAI : public ScriptedAI
    {
        npc_forgotten_one_DSAI(Creature* creature) : ScriptedAI(creature) { bossGUID = 0; }

        uint32 slicerTimer;
        uint64 bossGUID;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            me->PlayOneShotAnimKit(ANIM_KIT_EMERGE);
            slicerTimer = urand(4000, 6000);
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
            if (slicerTimer <= diff)
            {
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
                me->SetInCombatWithZone();

                if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                    me->CastSpell(target, SPELL_PSYCHIC_SLICE, true);
                slicerTimer = 3000; // guessing for now
            }
            else slicerTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class spell_gen_color_combination : public SpellScriptLoader
{
    public:
        spell_gen_color_combination() : SpellScriptLoader("spell_gen_color_combination") { }

        class spell_gen_color_combination_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_color_combination_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                Unit * caster = GetCaster();
                const SpellEntry * spelProto = GetSpellInfo();

                if (!caster || !spelProto)
                    return;

                for (std::list<Unit*>::iterator it = unitList.begin(); it != unitList.end();)
                {
                    if ((*it)->ToCreature() == NULL)
                    {
                        it = unitList.erase(it);
                        continue;
                    }

                    switch ((*it)->GetEntry())
                    {
                        case CRIMSON_GLOBULE_ENTRY:
                        case COBALT_GLOBULE_ENTRY:
                        case GLOWING_GLOBULE_ENTRY:
                        case DARK_GLOBULE_ENTRY:
                        case ACID_GLOBULE_ENTRY:
                        case SHADOWED_GLOBULE_ENTRY:
                            it++;
                            break;
                        default:
                            it = unitList.erase(it);
                            break;
                    }
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_color_combination_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);// 7
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_color_combination_SpellScript();
        }
};

class spell_gen_searing_blood : public SpellScriptLoader
{
public:
    spell_gen_searing_blood() : SpellScriptLoader("spell_gen_searing_blood") {}

    class spell_gen_searing_blood_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_gen_searing_blood_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hit_unit = GetHitUnit();

            if (!caster || !hit_unit )
                return;

            float distance = caster->GetExactDist2d(hit_unit->GetPositionX(),hit_unit->GetPositionY());

            if (distance <= 10.0f)
                return;

           if (const SpellRadiusEntry *radiusEntry = sSpellRadiusStore.LookupEntry(GetSpellInfo()->EffectRadiusIndex[EFFECT_0]))
                SetHitDamage(int32(GetHitDamage() * (1.0f + distance / radiusEntry->RadiusMax)));
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_gen_searing_blood_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_gen_searing_blood_SpellScript();
    }
};

void AddSC_boss_yorsahj_the_unsleeping()
{
    new boss_yorsahj_the_unsleeping(); // 55312
    new npc_yorsahj_blood_globule(); // 55862,55863,55864,55865,55866,55867
    new npc_forgotten_one_DS(); // 56265
    new npc_mana_void_DS(); // 56231
    new spell_gen_color_combination(); // 105420,105435,105436,105437,105439,105440
    new spell_gen_searing_blood(); // 105033,108356,108357,108358
}

// select * from creature_template where entry in (55312,55862,55863,55864,55865,55866,55867,56265,56231)

/*
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105420, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105435, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105436, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105437, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105439, 'spell_gen_color_combination');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105440, 'spell_gen_color_combination');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105033, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108356, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108357, 'spell_gen_searing_blood');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (108358, 'spell_gen_searing_blood');
*/