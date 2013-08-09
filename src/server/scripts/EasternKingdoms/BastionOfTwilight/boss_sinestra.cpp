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
#include "bastion_of_twilight.h"

#define MIDDLE_X -985.25f
#define MIDDLE_Y -777.516f
#define MIDDLE_Z 438.594177f

enum Spells
{
    /***** SINESTRA'S ABLITIES *******/
    SPELL_DRAINED              = 89350,  // -40 % dmg od sinestry v prvej faze
    SPELL_FLAME_BREATH         = 92944,  // Aoe
    SPELL_WRACK                = 92955,  // Dotka
    SPELL_TWILIGHT_BLAST       = 89280,  // Ak nie je niktio v melee range sinestry zacne castit TB
    SPELL_MANA_BARRIER         = 87299,  // Buff - Healne do full
    SPELL_TWILIGHT_POWER       = 87220,  // Po neskutocne dlhom hladani konecne spravny spell
    SPELL_TWILIGHT_EXTINCTION  = 86227,  // only viusal 15 s cast
    TWILIGHT_EXTINCTION_DMG    = 87945,  // 500k dmg instant
    SPELL_TWILIGHT_CARAPACE    = 87654,

    /****** CALENOVE ABILITY *********/
    SPELL_FIERY_RESOLVE        = 87221,  // viusal beam na stalkera
    SPELL_ESSENCE_OF_RED       = 87946,  // +100% haste buff + mana regen
    SPELL_FIERY_BARRIER_BUF    = 87231,  // buff duration 1 s (-95% shadow dmg)
    SPELL_FIERRY_BARRIER       = 95791,  // visual bubla
    SPELL_PYRRHIC_FOCUS        = 87323,  // Nefunkcny self buff (nutny workaround v kode)

    /******** SHADOW ORBY ***********/
    SPELL_TWILIGHT_SLICER      = 92851, // Beam + shadow dmg
    SPELL_SHADOW_PULSE         = 92957, // AoE dmg orbu

    /******** WHELP_PUDDLE **********/
    SPELL_TWILIGHT_ESSENCE      = 89284, // cast na seba ploska dava dmg sama
    INCREASE_PUDDLE             = 92952, // increase model size
    TWILIGHT_ESSENCE_AOE        = 88146, // aoe dmg

    SPELL_TWILIGHTT_SPIT       = 89299, // DOT + zvyseny shadow dmg

    /******* TWILIGHT DARKE **********/
    SPELL_TWILIGHT_BREATH      = 93544, // Frontal cone shadow dmg
    SPELL_ABSORB_ESSENCE       = 90107, // +10% hp a dmg buff

/****** TWILIGHT_SPITECALLER *********/
    SPELL_UNLEASH_ESSENCE      = 90028, //  10 % z max hp  kazdu sekundu pod dobu  10 sekund
    SPELL_INDOMITABLE_AOE      = 90045,
    SPELL_INDOMITABLE_DUMMY    = 90044,

    FLAMES_DND                  = 95823,
    TWILIGHT_INFUSION           = 87655,
};

enum Creatures
{
    SINESTRA_ENTRY                        = 45213,
    CREATURE_PULSING_TWILIGHT_EGG         = 46842,
    CREATURE_TWILIGHT_DRAKE               = 48436,
    CREATURE_TWILIGHT_WHELP               = 47265,
    CREATURE_CALEN                        = 46277,
    CREATURE_BARRIER_COMSMETIC_STALKER    = 51608,
    CREATURE_SHADOW_ORB1                  = 49862,
    CREATURE_SHADOW_ORB2                  = 49863,
    CREATURE_WHELP_AOE_PUDDLE             = 47058,
    CREATURE_TWILIGHT_FLAMES              = 407189,
    CREATURE_TWILIGHT_SPITECALLER         = 48415,
};

enum actions
{
    DO_REMOVE       = 70000, // Just to be sure that values are greater than 60000 -> duration of wrack
    DO_CHANGE_PHASE = 70001,
    DO_CALEN_DIED   = 70002,
    DO_WIN          = 70003,
};

# define NEVER  (4294967295) // used as "delayed" timer ( max uint32 value)

class boss_sinestra : public CreatureScript
{
public:
    boss_sinestra() : CreatureScript("boss_sinestra") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_sinestraAI (creature);
    }

    struct boss_sinestraAI : public ScriptedAI
    {
        boss_sinestraAI(Creature* creature) : ScriptedAI(creature),Summons(creature)
        {
            instance = creature->GetInstanceScript();

            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);

            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            me->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            me->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
            me->ApplySpellImmune(0, IMMUNITY_ID, 77606, true); // Dark Simulacrum 
        }

        InstanceScript* instance;
        SummonList Summons;

        uint32 Animation_timer;
        uint32 Flame_breath_timer;
        uint32 Wrack_timer;
        uint32 CheckTimer;
        uint32 PHASE;
        uint32 Drake_timer;
        uint32 Delay_timer;
        uint32 Spitecaller_timer;
        uint32 Beam_timer;
        uint32 Shadow_orb_timer;
        uint32 Spawn_calen_timer;
        uint32 Respawn_flames_timer;
        uint32 Voice_losing_time;
        uint32 Whelps_timer;
        uint32 Repeat_timer;
        bool may_cast_extinction;
        bool was_used, was_blasted,flamed;
        bool phrase1,phrase2,phrase3;

        uint32 eggs_dead;

        void Reset()
        {
            PHASE = 0;
            if (instance)
            {
                instance->SetData(DATA_SINESTRA, NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            eggs_dead = 0;
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,0.001f); // Avoid aggro on wipe
            Animation_timer = 11000;// Emerge animation time
            Flame_breath_timer = Animation_timer + 21000;
            Wrack_timer = Animation_timer + 15000;
            CheckTimer = Animation_timer + 4000;
            Shadow_orb_timer = Animation_timer + 25000; //first orbs after 25 seconds
            Whelps_timer = Animation_timer + 16000;
            was_used = was_blasted = may_cast_extinction = flamed = phrase1 = phrase2 = phrase3 = false;
            me->SetVisible(false);
            DoCast(me,SPELL_DRAINED,true);// - 40 % dmg reduction in first phase
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void JustSummoned(Creature* summon)
        {
            Summons.push_back(summon->GetGUID());
        }

        void DoAction(const int32 param) // Change phase after both eggs are dead
        {
            if(param == DO_CHANGE_PHASE)
            {
                eggs_dead ++;
            }
            else if (param == DO_CALEN_DIED)
            {
                eggs_dead = 2; // Lets continue to PHASE 3 if Calen died
            }
            else // Set duration for wrack
            {
                if(param)
                    SpreadWrack(param);
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            if (PHASE == 3)
                damage = damage * 1.2;

            if(!me->HasAura(SPELL_MANA_BARRIER)) // No barrier no deal
                return;

            me->ModifyPower(POWER_MANA,- ((int32)damage) );

            if(me->GetPower(POWER_MANA) <= (me->GetMaxPower(POWER_MANA)*0.27) ) // If under 27 % of mana
            {
                me->RemoveAurasDueToSpell(SPELL_MANA_BARRIER);
                Repeat_timer = 30000;

                me->MonsterTextEmote("The barrier protecting the Pulsing Twilight Eggs dissipates as Sinestra harnesses their power.", 0, true);

                std::list<Creature*> eggs;
                me->GetCreatureListWithEntryInGrid(eggs, CREATURE_PULSING_TWILIGHT_EGG, 500.0f);

                for (std::list<Creature*>::iterator itr = eggs.begin(); itr != eggs.end(); ++itr)
                    (*itr)->AI()->DoAction(DO_REMOVE); // Remove barrier from eggs
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->SetUInt64Value(UNIT_FIELD_HEALTH,(uint64) (me->GetMaxHealth() * 0.6 )); // Starting with 60% of HP
            me->ModifyPower(POWER_MANA, me->GetMaxPower(POWER_MANA)); // Fill mana to full
            me->SetVisible(true);
            me->SummonGameObject(207679,-1032.994f,-830.444f,441.0f,2.954f,0,0,0,0,0); // Fire wall
            me->SendPlaySound(20429, false);
            me->CastSpell(me,75764,true); // Visual emerge animation
            if (instance)
            {
                instance->SetData(DATA_SINESTRA, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }
        }

        void JustDied(Unit* /*Killer*/)
        {
            Summons.DespawnAll();

            me->MonsterYell("Deathwing! I have fallen.... The brood... is lost.", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20200, false);
            me->ForcedDespawn(15000);
            me->SummonGameObject(208045,-964.64f,-752.1f,438.6f,4.18f,0,0,0,0,0);// Summon Cache for loot

            GameObject * gowall = me->FindNearestGameObject(207679,200.0f); // Remove Fire wall
            if(gowall)
                gowall->Delete();

            if (instance)
            {
                instance->SetData(DATA_SINESTRA, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }
        }

        void SpreadWrack(int32 restDuration)
        {
            std::list<Player*> spread_targets;
            std::list<Player*> backup_targets;

            std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
            for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
            {
                if ( Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                {
                    Player * p = unit->ToPlayer();
                    if (p)
                    {
                        if (   p->GetActiveTalentBranchSpec() != SPEC_WARRIOR_PROTECTION && p->GetActiveTalentBranchSpec() != SPEC_PALADIN_PROTECTION
                            && ( p->GetActiveTalentBranchSpec() != SPEC_DRUID_FERAL && !p->HasAura(5487) ) && p->GetActiveTalentBranchSpec() != SPEC_DK_BLOOD)
                        {
                            if (p->HasAura(89421) || p->HasAura(92955)) // If have wrack push him to backup player list
                                backup_targets.push_back(p);
                            else
                                spread_targets.push_back(p); // Players without wrack
                        }
                    }
                }
            }

            uint8 valid_players = 0;

            if(!spread_targets.empty())
            {
                for (std::list<Player*>::const_iterator itr = spread_targets.begin(); itr!= spread_targets.end(); ++itr)
                {
                    if(valid_players == 2)
                        break;

                    if ((*itr) && (*itr)->IsInWorld())
                    {
                        valid_players++;
                        me->CastSpell((*itr),89421,true); // Wrack
                        Aura * a = (*itr)->GetAura(89421);
                        if(a)
                            a->SetDuration(restDuration);
                    }
                }
            }

            if(!backup_targets.empty() && valid_players < 2)
            {
                for (std::list<Player*>::const_iterator itr = backup_targets.begin(); itr!= backup_targets.end(); ++itr)
                {
                    if(valid_players == 2)
                        break;

                    if ((*itr) && (*itr)->IsInWorld())
                    {
                        valid_players++;
                        me->CastSpell((*itr),89421,true); // Wrack
                        Aura * a = (*itr)->GetAura(89421);
                        if(a)
                            a->SetDuration(restDuration);
                    }
                }
            }
        }


        void KilledUnit(Unit * victim)
        {
            if(!victim && !victim->ToPlayer())
                return;

            if(urand(0,1))
            {
                me->MonsterYell("My brood will feed on your bones!", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20201, false);
            }
            else 
            {
                me->MonsterYell("Powerless...", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20202, false);
            }
        }

        void EnterEvadeMode() //Despawn vajec + flameov
        {
            Summons.DespawnAll();

            GameObject * gowall = me->FindNearestGameObject(207679,200.0f); // Remove Fire wall
            if(gowall)
                gowall->Delete();

            if (instance)
            {
                instance->SetData(DATA_SINESTRA, NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            ScriptedAI::EnterEvadeMode();
        }

        void SpawnShadowOrbs(void)
        {
            std::list<Player*> wrack_targets;

            std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
            for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
            {
                if ( Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                {
                    Player * p = unit->ToPlayer();
                    if (p && p->IsInWorld() == true)
                    {
                        if (   p->GetActiveTalentBranchSpec() != SPEC_WARRIOR_PROTECTION && p->GetActiveTalentBranchSpec() != SPEC_PALADIN_PROTECTION
                            && ( p->GetActiveTalentBranchSpec() != SPEC_DRUID_FERAL && !p->HasAura(5487) ) && p->GetActiveTalentBranchSpec() != SPEC_DK_BLOOD)
                        {
                            wrack_targets.push_back(p);
                        }
                    }
                }
            }

            if(!wrack_targets.empty() && wrack_targets.size() >= 2 )
            {
                std::list<Player*>::iterator j = wrack_targets.begin();
                advance(j, rand()%wrack_targets.size());
                if (*j && (*j)->IsInWorld() == true )
                    me->SummonCreature(CREATURE_SHADOW_ORB1,(*j)->GetPositionX(),(*j)->GetPositionY() + urand(0,5),(*j)->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);

                wrack_targets.erase(j);
                j = wrack_targets.begin();

                advance(j, rand()%wrack_targets.size());
                if (*j && (*j)->IsInWorld() == true )
                    me->SummonCreature(CREATURE_SHADOW_ORB2,(*j)->GetPositionX() + urand(0,5),(*j)->GetPositionY(),(*j)->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);

            }
            else
            {
                 Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true);

                 if(player)
                 {
                    me->SummonCreature(CREATURE_SHADOW_ORB1,player->GetPositionX(),player->GetPositionY() + urand(0,5),player->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);
                    me->SummonCreature(CREATURE_SHADOW_ORB2,player->GetPositionX() + urand(0,5),player->GetPositionY(),player->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);
                 }
            }
        }

        void CastWrack(void)
        {
            std::list<Player*> wrack_targets;

            std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
            for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
            {
                if ( Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid()))
                {
                    Player * p = unit->ToPlayer();
                    if (p && p->IsInWorld() == true)
                    {
                        if (   p->GetActiveTalentBranchSpec() != SPEC_WARRIOR_PROTECTION && p->GetActiveTalentBranchSpec() != SPEC_PALADIN_PROTECTION
                            && ( p->GetActiveTalentBranchSpec() != SPEC_DRUID_FERAL && !p->HasAura(5487) ) &&  p->GetActiveTalentBranchSpec() != SPEC_DK_BLOOD)
                        {
                            wrack_targets.push_back(p);
                        }
                    }
                }
            }

            if(!wrack_targets.empty())
            {
                std::list<Player*>::const_iterator j = wrack_targets.begin();
                advance(j, rand()%wrack_targets.size());
                if (*j && (*j)->IsInWorld() == true )
                    me->CastSpell(*j,SPELL_WRACK,true);
            }
            /*else
            {
                if ( Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                    me->CastSpell(player,SPELL_WRACK,true);
            }*/
        }

        void UpdateAI(const uint32 Diff)
        {
            if (!UpdateVictim())
                return;

            if (Animation_timer <= Diff && PHASE==0)
            {
                me->SetFloatValue(UNIT_FIELD_COMBATREACH,45.0f);
                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,60.0f);
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);

                me->MonsterYell("We were fools to entrust an imbecile like Cho'gall with such a sacred duty. I will deal with you intruders myself!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(20199, false);

                me->CastSpell(me,95855,false); // Visual call flames
                me->SummonCreature(CREATURE_PULSING_TWILIGHT_EGG,-900.450867f, -767.153381f, 441.451935f,3.45f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_PULSING_TWILIGHT_EGG,-995.201843f, -674.263855f, 440.069366f, 4.62f , TEMPSUMMON_CORPSE_DESPAWN, 0);

                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-904.473755f,-769.9f, 441.850739f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-928.732f, -774.8344f, 441.019073f, 0.0f , TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-995.78f,-731.838f,439.27356f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-998.61f,-699.241f,441.598297f, 0.0f , TEMPSUMMON_CORPSE_DESPAWN, 0);

                PHASE=1;
            }
            else Animation_timer-=Diff;

/*###################################### PHASE 1 ################################################*/
            if (PHASE == 1 )
            {
                if ( Whelps_timer <= Diff)
                {
                    float angle,dist,height;

                    for ( uint8 i = 0; i < 5; i++ ) // Summon 5 drakes
                    {
                        //Generate random angle,distance,height and spawn whelp on that position
                        angle = (float)urand(0,6) + 0.28f ; 
                        dist = (float) urand(40,60);
                        height = (float)urand(15,35);

                        me->SummonCreature(CREATURE_TWILIGHT_WHELP, MIDDLE_X + cos(angle) * dist, MIDDLE_Y + sin(angle) * dist, MIDDLE_Z + height ,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    }

                    Whelps_timer = 50000;
                }
                else Whelps_timer -= Diff;

                if (Flame_breath_timer <= Diff)
                {
                    me->CastSpell(me,92944,false); // Aoe flame breath
                    Flame_breath_timer = urand(20000,25000);
                }
                else Flame_breath_timer -= Diff;

            // WRACK
            if (Wrack_timer <= Diff)
            {
                CastWrack();
                Wrack_timer= 65000;
            }
            else Wrack_timer -= Diff;

            // CheckTimer - If tank is not in melee range, cast Twilight blast on random player
            if (CheckTimer <= Diff)
            {
                bool inMeleeRange = ( me->IsWithinMeleeRange(me->getVictim()) ) ? true : false;

                if (inMeleeRange == false)
                    if ( Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                        if (PHASE == 1 || PHASE == 3)
                            if (!me->IsNonMeleeSpellCasted(false))
                                me->CastSpell(target,SPELL_TWILIGHT_BLAST,false);

                    CheckTimer = 4000;
             }
             else CheckTimer -= Diff;


            if (Shadow_orb_timer <= Diff)
            {
                SpawnShadowOrbs();
                Shadow_orb_timer = 30000;
            }
            else Shadow_orb_timer-=Diff;

    }

    ///////  PRECHOD DO DRUHEJ FAZE  //////
    if(HealthBelowPct(30) && PHASE==1)
    {
        was_used = was_blasted = false;
        PHASE = 2;
        Beam_timer = 15000;
        Delay_timer= 10000;
        Drake_timer= 60000;
        Spawn_calen_timer = 16000;
        Spitecaller_timer = Spawn_calen_timer + 55000;
        Voice_losing_time = 90000 - 21000;
        Repeat_timer = NEVER;

        me->InterruptNonMeleeSpells(false);
        me->MonsterYell("I tire of this. Do you see this clutch amidst which you stand? I have nurtured the spark within them, but that life-force is and always will be mine. Behold, power beyond your comprehension!", LANG_UNIVERSAL, 0);
        me->SendPlaySound(20204, false);
        DoCast(me,SPELL_MANA_BARRIER); 
    }

// ############################# PHASE 2 ######################################### 

    if (PHASE == 2)
    {
        if (Delay_timer <= Diff) // After 6 seconds from entering PHASE2 can can Twilight Extinction
        {
            may_cast_extinction = true;
        }
        else Delay_timer-=Diff;


        if (may_cast_extinction == true)// Set hp to full during whole phase 2
            me->SetHealth(me->CountPctFromMaxHealth(100));

        if (was_used==false && may_cast_extinction)
        {
            if (!me->IsNonMeleeSpellCasted(false))
            {
                DoCast(me,SPELL_TWILIGHT_EXTINCTION); // Only 15 s visual cast
                Beam_timer = 15000 + 5000;
                was_used=true;
            }

        }

        if (was_used && !me->IsNonMeleeSpellCasted(false) && !was_blasted) // After 15 seconds
        {
            me->CastSpell(me,TWILIGHT_EXTINCTION_DMG,true); // Cast damage spell
            was_blasted=true;
        }

        if (Spawn_calen_timer <= Diff) // Spawn Stalker and Calen after 10 seconds
        {
            me->SummonCreature(CREATURE_BARRIER_COMSMETIC_STALKER,-988.927f,-783.37f,439.0f, 0.0f,TEMPSUMMON_CORPSE_DESPAWN,0);
            me->SummonCreature(CREATURE_CALEN,-1015.624207f,-815.707947f,438.593506f, 0.0f,TEMPSUMMON_CORPSE_DESPAWN,5000);
            Spawn_calen_timer = NEVER;
        }
        else Spawn_calen_timer -= Diff;


        if (Beam_timer <= Diff) // Visual cast of beamu on stalkera
        {
            if (Creature *pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 500.0f, true))
                me->CastSpell(pStalker,SPELL_TWILIGHT_POWER,false);

            Beam_timer = NEVER;
        }
        else Beam_timer -= Diff;


        if (Voice_losing_time <= Diff)
        {
            me->MonsterYell("You mistake this for weakness? Fool!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20203, false);

            Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 200, true);
            if (pStalker)
                pStalker->GetMotionMaster()->MovePoint(0,-995.16f,-793.48f,438.6f);

            Voice_losing_time = NEVER;
        }
        else Voice_losing_time -= Diff;

        if (Drake_timer <= Diff) // Spawn Twilight drake
        {
            me->SummonCreature(CREATURE_TWILIGHT_DRAKE,-1038.66f,-710.26f,461.78f, 5.14f,TEMPSUMMON_CORPSE_DESPAWN,0);
            Drake_timer = 45000;
        }
        else Drake_timer -= Diff;


        if (Spitecaller_timer <= Diff) // Spawn Twilight Spitecaller
        {
            me->SummonCreature(CREATURE_TWILIGHT_SPITECALLER,-1042.167f,-850.95f,445.74762f,1.03f);
            Spitecaller_timer = 55000;
        }
        else Spitecaller_timer -= Diff;

        if(Repeat_timer <= Diff )
        {
            me->CastSpell(me,SPELL_MANA_BARRIER,true);
            me->ModifyPower(POWER_MANA, me->GetMaxPower(POWER_MANA)); // Fill mana to full again
            Repeat_timer = NEVER;
        }
        else Repeat_timer -= Diff;

    }


/**************** SWITCHING TO PHASE 3 ********************/

        if(eggs_dead == 2 && PHASE==2) // Both eggs are dead
        {
            if(Creature * pCalen = me->FindNearestCreature(CREATURE_CALEN,200.0f,true) )
                pCalen->AI()->DoAction(DO_WIN);

            PHASE = 3;
            me->ModifyPower(POWER_MANA, me->GetMaxPower(POWER_MANA)); // Fill mana to full
            me->RemoveAurasDueToSpell(SPELL_DRAINED); // Remove damage reduction  debuff

            if(Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 500.0f, true))
                pStalker->ForcedDespawn();

            me->InterruptNonMeleeSpells(false);
            me->MonsterYell("Enough! Drawing upon this source will set us back months. You should feel honored to be worthy of its expenditure. Now... die!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20206, false);

            Flame_breath_timer = 25000;
            Wrack_timer = 15000;
            CheckTimer = 20000;
            Shadow_orb_timer = 30000;
            Respawn_flames_timer = 15000;
            Whelps_timer = 20000;
            flamed = false; // Spawn Twilight Flames again
        }

//############################ PHASE 3 ##############################/

        if (PHASE == 3)
        {
            if (Respawn_flames_timer <= Diff && !flamed) // Respawn twilight flames
            {
                me->CastSpell(me,95855 ,false); // Visual cast call flames

                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-904.473755f,-769.9f, 441.850739f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-928.732f, -774.8344f, 441.019073f, 0.0f , TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-995.78f,-731.838f,439.27356f,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                me->SummonCreature(CREATURE_TWILIGHT_FLAMES,-998.61f,-699.241f,441.598297f, 0.0f , TEMPSUMMON_CORPSE_DESPAWN, 0);
                flamed = true;
            }
            else Respawn_flames_timer -= Diff;

            // DIFFERENT PHRASES IN PHASE 3

            if (me->HealthBelowPct(78) && !phrase1)
            {
                me->MonsterYell("The energy infuse within my clutch is mine to reclaim!", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20208, false);
                phrase1=true;
            }

            if (me->HealthBelowPct(32) && !phrase2)
            {
                me->MonsterYell("SUFFER!", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20209, false);
                phrase2=true;
            }

            if (me->HealthBelowPct(8) && !phrase3)
            {
                me->MonsterYell("FEEL MY HATRED!", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20210, false);
                phrase3=true;
            }


/********** SAME AS PHASE 1 **********/

            if ( Whelps_timer <= Diff)
            {
                float angle,dist,height;

                for ( uint8 i = 0; i < 5; i++ ) // Summon 5 drakes
                {
                    //Generate random angle,distance,height and spawn whelp on that position
                    angle = (float)urand(0,6) + 0.28f ;
                    dist = (float) urand(40,60);
                    height = (float)urand(15,35);

                    me->SummonCreature(CREATURE_TWILIGHT_WHELP, MIDDLE_X + cos(angle) * dist, MIDDLE_Y + sin(angle) * dist, MIDDLE_Z + height ,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                }

                Whelps_timer = 50000;
            }
            else Whelps_timer -= Diff;

            // Flame breath
            if (Flame_breath_timer <= Diff)
            {
                me->CastSpell(me,92944,false);
                Flame_breath_timer = urand(20000,25000);
            }
            else Flame_breath_timer -= Diff;

            // Wrack
            if (Wrack_timer <= Diff)
            {
                CastWrack();
                Wrack_timer= 65000;
            }
            else Wrack_timer -= Diff;

            // CheckTimer - If tank is not in melee range, cast Twilight blast on random player
            if (CheckTimer <= Diff)
            {
                bool inMeleeRange = ( me->IsWithinMeleeRange(me->getVictim()) ) ? true : false;

                if (inMeleeRange == false)
                    if ( Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200.0f, true) )
                        if (PHASE == 1 || PHASE == 3)
                            if (!me->IsNonMeleeSpellCasted(false))
                                me->CastSpell(target,SPELL_TWILIGHT_BLAST,false);

                    CheckTimer = 4000;
             }
             else CheckTimer -= Diff;


            if (Shadow_orb_timer <= Diff)
            {
                SpawnShadowOrbs();
                Shadow_orb_timer = 30000;
            }
            else Shadow_orb_timer -= Diff;

        }
            if (PHASE != 0 && PHASE != 2)
                DoMeleeAttackIfReady();
       }
    };
};

class mob_shadow_orb_I : public CreatureScript
{
public:
    mob_shadow_orb_I() : CreatureScript("mob_shadow_orb_I") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shadow_orb_IAI (creature);
    }

    struct mob_shadow_orb_IAI : public ScriptedAI
    {
        mob_shadow_orb_IAI(Creature* creature) : ScriptedAI(creature)
        {
            Reset();
        }

        uint32 Pulse_timer;
        uint32 Despawn_timer;
        uint32 Beam_timer;
        uint32 Fixate_timer;
        uint32 Slicer_timer;

        void DamageTaken(Unit* attacker, uint32& damage)
        {
                damage = 0;
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            Fixate_timer = 500;
            Beam_timer = 1500;
            Pulse_timer = 5000;
            Despawn_timer = 15000;
            Slicer_timer = 5000;
            me->SetSpeed(MOVE_RUN,0.65f);
            me->SetInCombatWithZone();
        }

        bool HasHealingSpec(Player * p )
        {
            return ( p->GetActiveTalentBranchSpec() == SPEC_DRUID_RESTORATION || p->GetActiveTalentBranchSpec() == SPEC_PALADIN_HOLY ||  p->GetActiveTalentBranchSpec() == SPEC_PRIEST_DISCIPLINE ||
                     p->GetActiveTalentBranchSpec() == SPEC_PRIEST_HOLY || p->GetActiveTalentBranchSpec() == SPEC_SHAMAN_RESTORATION );
        }

        bool HasTankSpec(Player * p )
        {
            return ( p->GetActiveTalentBranchSpec() == SPEC_WARRIOR_PROTECTION || p->GetActiveTalentBranchSpec() == SPEC_PALADIN_PROTECTION ||  p->GetActiveTalentBranchSpec() == SPEC_DK_BLOOD 
                || ( p->GetActiveTalentBranchSpec() == SPEC_DRUID_FERAL && p->HasAura(5487) ) );
        }

        Player * GetFixateVictim(void)
        {
            std::list<Player*> player_list;

            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* pPlayer = itr->getSource();
                    if (pPlayer && !pPlayer->isGameMaster())
                        if (pPlayer->GetDistance(me) < 100.0f)
                            if ( HasHealingSpec(pPlayer) == false && HasTankSpec(pPlayer) == false) // Exclude healers and tanks
                            {
                                player_list.push_back(pPlayer);
                            }
                }
            }

            if (!player_list.empty())
            {
                std::list<Player*>::const_iterator j = player_list.begin();
                advance(j, rand()%player_list.size());
                return (*j);
            }
            else
            {
                if ( Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                    return player->ToPlayer();
            }

            return NULL;
        }

        void UpdateAI (const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->getVictim())
                me->getVictim()->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);

            if (Fixate_timer <= diff)
            {
                Player * fix_pl = GetFixateVictim();
                if(fix_pl)
                {
                    fix_pl->Say("Shadow orb on me !!!", LANG_UNIVERSAL);
                    me->TauntApply(fix_pl);
                    me->AddThreat(fix_pl,5000000.0f);
                }

                Fixate_timer = NEVER;
            }
            else Fixate_timer -= diff;

            if (Beam_timer <= diff)
            {
                if ( ! me->getVictim()->GetAura(35371,me->GetGUID()) )
                    me->CastSpell(me->getVictim(),35371,true); // White beam
                Beam_timer = 100;
            }
            else Beam_timer -= diff;

            if(Slicer_timer <= diff)
            {
                me->CastSpell(me,92852,true); // twilight slicer
                Slicer_timer = 300;
            }
            else Slicer_timer -= diff;

            if(Pulse_timer <= diff) // Po 3.5 sekundach sa orb uvolni a zacne nahanat hraca
            {
                if(Creature* orb2 = me->FindNearestCreature(CREATURE_SHADOW_ORB2,200.0f,true))
                    me->CastSpell(orb2,92851,true);

                me->clearUnitState(UNIT_STAT_CASTING);
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->CastSpell(me,SPELL_SHADOW_PULSE,true);

                Pulse_timer = NEVER;
            }
            else Pulse_timer -= diff;

            if(!me->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE)) 
            {
                Unit * victim = me->getVictim();
                if(victim)
                    me->GetMotionMaster()->MovePoint(0,victim->GetPositionX(),victim->GetPositionY(),victim->GetPositionZ());
            }

            if(Despawn_timer <= diff)
            {
                me->Kill(me);
                Despawn_timer = NEVER;
            }
            else Despawn_timer -= diff;

        }

    };
};

class mob_shadow_orb_II : public CreatureScript
{
public:
    mob_shadow_orb_II() : CreatureScript("mob_shadow_orb_II") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_shadow_orb_IIAI (creature);
    }

    struct mob_shadow_orb_IIAI : public ScriptedAI
    {
        mob_shadow_orb_IIAI(Creature* creature) : ScriptedAI(creature)
        {
            Reset();
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true); // For sure ( but they are unselectable so not neccessary )
        }

        uint32 Pulse_timer;
        uint32 Beam_timer;
        uint32 Despawn_timer;
        uint32 Fixate_timer;


        void DamageTaken(Unit* attacker, uint32& damage)
        {
                damage = 0;
        }

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            Fixate_timer = 1000;
            Beam_timer = 1500;
            Pulse_timer = 5000;
            Despawn_timer = 15000;
            me->SetSpeed(MOVE_RUN,0.65f);
            me->SetInCombatWithZone();
        }

        bool IsOrb1Victim(Player * p)
        {
            if (Creature * orb1 = me->FindNearestCreature(CREATURE_SHADOW_ORB1,200.0f,true) )
                if( orb1->getVictim() && orb1->getVictim() == p)
                    return true;
            return false;
        }

        bool HasTankSpec(Player * p )
        {
            return ( p->GetActiveTalentBranchSpec() == SPEC_WARRIOR_PROTECTION || p->GetActiveTalentBranchSpec() == SPEC_PALADIN_PROTECTION ||  p->GetActiveTalentBranchSpec() == SPEC_DK_BLOOD 
                || ( p->GetActiveTalentBranchSpec() == SPEC_DRUID_FERAL && p->HasAura(5487) ) );
        }

        Player * GetFixateVictim(void)
        {
            std::list<Player*> player_list;

            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.isEmpty())
            {
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* pPlayer = itr->getSource();
                    if (pPlayer && !pPlayer->isGameMaster())
                        if (pPlayer->GetDistance(me) < 100.0f)
                            if (HasTankSpec(pPlayer) == false && IsOrb1Victim(pPlayer) == false ) // Exclude tanks and orb1 victim
                            {
                                player_list.push_back(pPlayer);
                            }
                }
            }

            if (!player_list.empty())
            {
                std::list<Player*>::const_iterator j = player_list.begin();
                advance(j, rand()%player_list.size());
                return (*j);
            }
            else
            {
                if ( Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                    return player->ToPlayer();
            }

            return NULL;
        }

        void UpdateAI (const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->getVictim())
                me->getVictim()->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);

            if (Fixate_timer <= diff)
            {
                Player * fix_pl = GetFixateVictim();
                if(fix_pl)
                {
                    fix_pl->Say("Shadow orb on me !!!", LANG_UNIVERSAL);
                    me->TauntApply(fix_pl);
                    me->AddThreat(fix_pl,5000000.0f);
                }

                Fixate_timer = NEVER;
            }
            else Fixate_timer -= diff;

            if (Pulse_timer <= diff) // Po 3.5 sekundach sa orb uvolni a zacne nahanat hraca
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->CastSpell(me,SPELL_SHADOW_PULSE,true);
                Pulse_timer = NEVER;
            }
            else Pulse_timer -= diff;

            if (Beam_timer <= diff)
            {
                if ( ! me->GetAura(35371,me->getVictim()->GetGUID()) )
                    me->getVictim()->CastSpell(me,35371,true); // White beam

                Beam_timer = 100;
            }
            else Beam_timer -= diff;

            if(!me->HasFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE)) 
            {
                Unit * victim = me->getVictim();
                if(victim)
                    me->GetMotionMaster()->MovePoint(0,victim->GetPositionX(),victim->GetPositionY(),victim->GetPositionZ());
            }

            if(Despawn_timer <= diff)
            {
                me->Kill(me);
                Despawn_timer = NEVER;
            }
            else Despawn_timer -=diff;

        }

    };
};

/************ TWILIGHT WHELP AI ****************************/
class twilight_whelp : public CreatureScript
{
public:
   twilight_whelp() : CreatureScript("twilight_whelp") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new twilight_whelpAI (creature);
    }

    struct twilight_whelpAI : public ScriptedAI
    {
        twilight_whelpAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID = 0;
            summonerEntry = 0;
        }

        uint64 summonerGUID;
        uint32 summonerEntry;

        void DamageTaken(Unit* attacker, uint32& damage)
        {
                damage = damage *1.2;
        }

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, false);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
            me->SetInCombatWithZone();
            DoZoneInCombat(me,100.0f);
            me->SetSpeed(MOVE_FLIGHT, 1.4f, true);

            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
            {
                me->AddThreat(target, 1.0f);
                me->GetMotionMaster()->MoveChase(target);
            }
        }

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
            {
                summonerGUID = pSummoner->GetGUID();
                summonerEntry = pSummoner->ToCreature()->GetEntry();
            }
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType typeOfDamage) // Only 10 HC functionality
        {
            if (typeOfDamage == DIRECT_DAMAGE )
            {
                uint32 absorbAmount = 0;
                Unit::AuraEffectList const& auraAbsorbList = victim->GetAuraEffectsByType(SPELL_AURA_SCHOOL_ABSORB);
                for (Unit::AuraEffectList::const_iterator i = auraAbsorbList.begin(); i != auraAbsorbList.end(); ++i)
                    absorbAmount += (uint32((*i)->GetAmount()));

                if (absorbAmount >= damage) // Dont cast if damage was absorbed
                    return;

                uint32 stack_number = victim->GetAuraCount(SPELL_TWILIGHTT_SPIT); // save stacks of spit on target
                me->CastSpell(victim,SPELL_TWILIGHTT_SPIT,true); // Cast spell, cause we want damage efffect of that spell
                victim->RemoveAura(SPELL_TWILIGHTT_SPIT); // do not use removeaurasduetospell !!!! can cycle server due to wrack mechanic
                if(stack_number < 99 )
                    me->SetAuraStack(SPELL_TWILIGHTT_SPIT,victim,stack_number + 1 );
            }
        }

        void JustDied (Unit * killed)  // Spawn whelp puddle after dead
        {
            if (summonerEntry == SINESTRA_ENTRY)
            {
                if(Unit * pSinestra = Unit::GetUnit(*me,summonerGUID))
                    pSinestra->SummonCreature(CREATURE_WHELP_AOE_PUDDLE,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(), 0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
            }
            me->RemoveCorpse();
        }

        void UpdateAI (const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if(me->getVictim())
                me->getVictim()->RemoveAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);

            DoMeleeAttackIfReady();
        }
    };
};

/************** TWILIGHT DRAKE AI **********************************/
class mob_Twilight_Drake : public CreatureScript
{
public:
    mob_Twilight_Drake() : CreatureScript("mob_Twilight_Drake") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_Twilight_DrakeAI (creature);
    }

    struct mob_Twilight_DrakeAI : public ScriptedAI
    {
        mob_Twilight_DrakeAI(Creature* creature) : ScriptedAI(creature){ }

        uint32 Breath_timer;
        uint32 flight_timer;
        bool landed;

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);

            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(0,-1010.7f,-762.098f,439.644012f);
            flight_timer = 8000;
            Breath_timer = flight_timer + 10000;
            landed = false;
        }

        void UpdateAI (const uint32 diff)
        {
            if (Breath_timer <= diff) // Kazdych +- 10 sekund cast frontal cone shadow dmg
            {
                me->CastSpell(me->getVictim(),SPELL_TWILIGHT_BREATH,false);
                Breath_timer = 10000;
            }
            else Breath_timer -= diff;

            if (flight_timer <= diff && !landed)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_AGGRESSIVE);
                DoZoneInCombat(me,100.0f);

                landed = true;
            }
            else flight_timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

/******************* WHELP_AOE_PUDDLE AI ******************************/
 
class Whelp_puddle : public CreatureScript
{
public:
    Whelp_puddle() : CreatureScript("Whelp_puddle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Whelp_puddleAI (creature);
    }

    struct Whelp_puddleAI : public ScriptedAI
    {
        Whelp_puddleAI(Creature* creature) : ScriptedAI(creature) {}

        uint64 summonerGUID;
        uint32 growing_timer;
        uint32 delay_spawn_timer;
        uint32 aoe_damage_timer;
        bool summoned_whelps;
        float stacks;
        float boundig_radius;

        void Reset()
        {
            stacks = 1;
            boundig_radius = 1.5;
            summoned_whelps = false;
            delay_spawn_timer = 2000 + urand(1000,2000);
            aoe_damage_timer = 2000;
            growing_timer = 5000;
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,boundig_radius);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,boundig_radius);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me,SPELL_TWILIGHT_ESSENCE,true); // Visual puddle
        }

        void MoveInLineOfSight(Unit * who) 
        {
            if( who && who->ToCreature()  && who->ToCreature()->GetEntry() == CREATURE_TWILIGHT_DRAKE)
                if(me->IsWithinMeleeRange(who))
                {
                    who->CastSpell(who,SPELL_ABSORB_ESSENCE,true);
                    me->ForcedDespawn();
                }
        }

        void UpdateAI (const uint32 diff)
        {
            if (growing_timer <= diff )
            {
                me->CastSpell(me,INCREASE_PUDDLE,true); // Increase size by 15 %
                boundig_radius += 0.225f;
                me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,boundig_radius);
                stacks++;
                growing_timer = 10000;
            }
            else growing_timer -= diff;

            if ( aoe_damage_timer <= diff) // Manual aoe dmg, due to increasing spell radius
            {
                me->CastCustomSpell(TWILIGHT_ESSENCE_AOE, SPELLVALUE_RADIUS_MOD,(10000 + stacks * 1500)); // 100% + 15 % stacks
                aoe_damage_timer = 2000;
            }
            else aoe_damage_timer -= diff;

            if (delay_spawn_timer <=diff && !summoned_whelps) // After few seconds spawn whelp on puddle position
            {
                me->SummonCreature(CREATURE_TWILIGHT_WHELP,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,1000);
                summoned_whelps = true;
            }
            else delay_spawn_timer-=diff;
        }
    };
};

/*******************CALEN AI ******************************/
class mob_calen : public CreatureScript
{
public:
    mob_calen() : CreatureScript("mob_calen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_calenAI (creature);
    }

    struct mob_calenAI : public ScriptedAI
    {
        mob_calenAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            Reset();
            me->setPowerType(POWER_MANA);
            me->SetMaxPower(POWER_MANA,2000000);
            me->ModifyPower(POWER_MANA, me->GetMaxPower(POWER_MANA));
        }

        InstanceScript * instance;
        uint32 Desummon_Timer;
        uint32 Beam_timer;
        uint32 Ticking_timer;
        uint32 Remove_barrier;
        uint32 Voice_timer;
        uint32 Winning_timer;
        uint32 Talk_timer;
        bool check_health;

        void Reset()
        {
            Beam_timer = 18000;
            Desummon_Timer = NEVER; // 90000
            Winning_timer = 47200;
            Talk_timer = 1000;
            Remove_barrier = 20000;
            Ticking_timer = NEVER;
            Voice_timer = NEVER; // 74000
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT); // Stop HP regeneration
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            check_health = false;
        }

        void JustDied(Unit* killer)
        {
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            if(killer == me)
            {
                me->MonsterYell("All is lost.... Forgive me, my Queen....", LANG_UNIVERSAL, 0);
                me->SendPlaySound(21598, false);
            }

            if (Creature* pSinestra = me->FindNearestCreature(SINESTRA_ENTRY, 200.0f, true))
                pSinestra->AI()->DoAction(DO_CALEN_DIED);

            if (Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 200.0f, true))
                pStalker->ForcedDespawn();
        }

        void DoAction(const int32 action) // This will be called if dummy aoe spell hits a player ( in melee range ) - spellscript
        {
            if(action == DO_WIN)
            {
                Voice_timer = 8000;
                Desummon_Timer = 16000;
            }
        }

        void UpdateAI (const uint32 diff)
        {
            // Pri resete nepocut hlasku calena tak som to musel dat s odstupom
            if (Talk_timer <= diff)
            {
                if (instance)
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                me->CastSpell(me,SPELL_FIERRY_BARRIER,true);
                me->CastSpell(me,96431,true); // DND
                me->CastSpell(me,87229,true); // - 99% reduction shadow reduction na friendly units v okoli
                me->MonsterYell("Heroes! You are not alone in this dark place!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(21588, false);
                Talk_timer = NEVER;
            }
            else Talk_timer -= diff;

            if (Winning_timer <= diff)
            {
                me->MonsterYell("You are weakening, Sintharia! Accept the inevitable!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(21593, false);

                Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 200, true);
                if(pStalker)
                    pStalker->GetMotionMaster()->MovePoint(0,-979.4f,-772.18f,438.6f);

                Winning_timer = NEVER;
            }
            else Winning_timer -= diff;

            if (Remove_barrier <= diff)
            {
                me->MonsterYell("Sintharia! Your master owes me a great debt... one that I intend to extract from his consort's hide!", LANG_UNIVERSAL, 0);
                me->SendPlaySound(21590, false);
                me->RemoveAurasDueToSpell(SPELL_FIERRY_BARRIER); // visual barrier
                me->RemoveAurasDueToSpell(96431); // DND viusal
                me->RemoveAurasDueToSpell(87229); // shadow reduction buff
                Remove_barrier = NEVER;
            }
            else Remove_barrier -= diff;


            if (Beam_timer <= diff)
            {
                DoCast(me,SPELL_PYRRHIC_FOCUS,true);
                Ticking_timer = 1200;
                Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 200, true);
                if(pStalker)
                   DoCast(pStalker,SPELL_FIERY_RESOLVE);
                Beam_timer = NEVER;
            }
            else Beam_timer -= diff;

            if (Ticking_timer <= diff)
            {
                Ticking_timer = 1000;
                //if( !me->hasUnitState(UNIT_STAT_CASTING) )
                   // return;

                me->DealDamage(me, me->GetMaxHealth()*0.01f); // Kazdu sekundu uberem Calenovi 1 % jeho hp
                int32 mod = me->GetMaxPower(POWER_MANA) / 100;
                mod *= 1;
                me->ModifyPower(POWER_MANA,-mod);
                

                if (HealthBelowPct(20) && !check_health)
                {
                    if (Voice_timer < 100000)
                    {
                        me->MonsterYell("Heroes! My power wanes...", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(21589, false);
                    }
                    check_health=true;
                }
            }
            else Ticking_timer -= diff;

            if (Voice_timer <= diff)
            {
                me->MonsterYell("The fires dim, champions... Take this, the last of my power. Succeed where I have failed... Avenge me. Avenge the world...", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(21591, false);
                Voice_timer = NEVER;
            }
            else Voice_timer -= diff;

            if (Desummon_Timer <= diff)
            {
                me->InterruptNonMeleeSpells(false);
                me->CastSpell(me,SPELL_ESSENCE_OF_RED,true); // Aoe buff na 100% haste + mana regen

                if (Creature* pStalker = me->FindNearestCreature(CREATURE_BARRIER_COMSMETIC_STALKER, 500, true))
                    pStalker->ForcedDespawn();
                Desummon_Timer = NEVER;
                if (Creature* pSinestra = me->FindNearestCreature(SINESTRA_ENTRY, 200.0f, true))
                    pSinestra->Kill(me);
            }

            else Desummon_Timer -= diff;
        }
    };
};


/******************* TWILIGHT EGG AI ******************************/
class Twilight_egg : public CreatureScript
{
public:
    Twilight_egg() : CreatureScript("Twilight_egg") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Twilight_eggAI (creature);
    }

    struct Twilight_eggAI : public ScriptedAI
    {
        Twilight_eggAI(Creature* creature) : ScriptedAI(creature)
        {
            summonerGUID = 0;
        }

        uint64 summonerGUID;
        uint32 Reapply_timer;
        bool can_remove;

        void IsSummonedBy(Unit* pSummoner)
        {
            if (pSummoner && pSummoner->ToCreature())
                summonerGUID = pSummoner->GetGUID();
        }

        void Reset()
        {
            can_remove = false;
            Reapply_timer = NEVER;
            me->SetReactState(REACT_PASSIVE);
            me->SetInCombatWithZone();
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);

            me->CastSpell(me,SPELL_TWILIGHT_CARAPACE,true); // Absorb
        }

        void JustDied (Unit * killed)
        {
            if (summonerGUID == 0 )
                return;

            if (Unit* summoner = Unit::GetUnit(*me,summonerGUID))
                summoner->ToCreature()->AI()->DoAction(DO_CHANGE_PHASE);
        }

        void DoAction(const int32 param)
        {
            if(param == DO_REMOVE)
            {
                Reapply_timer = 30000;
                can_remove = true;

                me->RemoveAurasDueToSpell(SPELL_TWILIGHT_CARAPACE);

                if(summonerGUID)
                    if (Unit* summoner = Unit::GetUnit(*me,summonerGUID))
                        me->CastSpell(summoner,TWILIGHT_INFUSION,false); // Purple beam
            }
        }


        void UpdateAI (const uint32 diff)
        {
            if(Reapply_timer <= diff && can_remove )
            {
                me->CastSpell(me,SPELL_TWILIGHT_CARAPACE,true); // Reapply absorb
                me->SetFullHealth();
                Reapply_timer = NEVER;
                can_remove = false;
            }
            else Reapply_timer -= diff;
        }
    };
};


class mob_Twilight_flames : public CreatureScript
{
public:
   mob_Twilight_flames() : CreatureScript("mob_Twilight_flames") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_Twilight_flamesAI (creature);
    }

    struct mob_Twilight_flamesAI : public ScriptedAI
    {
        mob_Twilight_flamesAI(Creature* creature) : ScriptedAI(creature) 
        {
            summonerGUID = 0;
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
        }

        uint32 Burn_timer;
        uint64 summonerGUID;
        bool despawned;

        void IsSummonedBy(Unit* pSummoner)
        {
            if(pSummoner && pSummoner->ToCreature())
                summonerGUID = pSummoner->GetGUID();
        }

        void Reset()
        {
            Burn_timer = 2800;
            despawned = false;
        }

         void UpdateAI (const uint32 diff)
        {
            if(despawned == false && summonerGUID)
                if (Unit* summoner = Unit::GetUnit(*me,summonerGUID))
                    if(summoner->HealthBelowPct(30) && summoner->HasAura(SPELL_DRAINED))
                    {
                        me->ForcedDespawn(20000);
                        despawned = true;
                        return;
                    }

            if (Burn_timer <= diff)
            {
                me->CastSpell(me,FLAMES_DND,true); // visual + aoe dmg
                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
                Burn_timer = NEVER;
            }
            else Burn_timer -= diff;
        }
    };
};

/**************** TWILIGHT SPITECALLER **************************/
class mob_Twilight_spitecaller : public CreatureScript
{
public:
   mob_Twilight_spitecaller() : CreatureScript("mob_Twilight_spitecaller") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_Twilight_spitecallerAI (creature);
    }

    struct mob_Twilight_spitecallerAI : public ScriptedAI
    {
        mob_Twilight_spitecallerAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 Walk_timer;
        uint32 Dot_timer;

        void SpellHit(Unit* caster, const SpellEntry* spell) // HORRIBLE CONDITIONS ( Blizzard facepalm )
        {
            if(caster == me ) // Anti recursion check, due to indomitable spell :P
                return;

            for (uint32 i = 0 ; i < MAX_SPELL_EFFECTS; i ++) // Interrupt spell without MECHANIC_INTERRUPT are fine
            {
                if (spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST && spell->EffectMechanic[i] != MECHANIC_INTERRUPT)
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }
            }

            if ( spell->AppliesAuraType(SPELL_AURA_MOD_FEAR) ) // Fear like spells like fine
            {
                me->InterruptNonMeleeSpells(false);
                me->RemoveAurasDueToSpell(spell->Id);
                return;
            }

            if ( spell->Id == 1776 || spell->Id == 20066 || spell->Id == 19503 || spell->SpellIconID == 3440) // Gouge,Repentance,Scatter shot, Blind
            {
                me->InterruptNonMeleeSpells(false);
                me->RemoveAurasDueToSpell(spell->Id);
                return;
            }

            if ( (spell->AttributesEx & SPELL_ATTR0_BREAKABLE_BY_DAMAGE) || (spell->AttributesEx & SPELL_ATTR0_STOP_ATTACK_TARGET) ) // E.g Hex,Hibernate ( is fine )
            {
                if ( !spell->AppliesAuraType(SPELL_AURA_MOD_CONFUSE) && !spell->AppliesAuraType(SPELL_AURA_MOD_STUN) ) // Not poly e.g, not allowed
                {
                    me->InterruptNonMeleeSpells(false);
                    me->RemoveAurasDueToSpell(spell->Id);
                    return;
                }
            }


            if (spell->AppliesAuraType(SPELL_AURA_MOD_STUN) || spell->AppliesAuraType(SPELL_AURA_MOD_SILENCE) ) // Stun - Silence like spells trigger indomtibale
            {
                me->RemoveAurasDueToSpell(spell->Id);
                me->InterruptNonMeleeSpells(false);
                me->CastSpell(me,SPELL_INDOMITABLE_AOE,true); // That will hurt :)
                me->CastSpell(me,SPELL_INDOMITABLE_DUMMY,true); // Dummy ?
            }
        }

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, 77606, true); // Dark simulacrum
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);

            me->SetSpeed(MOVE_WALK, 1.1f, true);
            me->GetMotionMaster()->MovePoint(0,-1014.179f,-808.09f,439.0f);

            Walk_timer = 10000; // 10 seconds till he walks to place
            Dot_timer = Walk_timer + urand(8000,9000);
        }

        void UpdateAI (const uint32 diff)
        {
            if (Walk_timer <= diff ) // Aggro after 15 seconds
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
                me->SetSpeed(MOVE_RUN,1.0f,true);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();

                if (Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                    me->GetMotionMaster()->MoveChase(player);

                Walk_timer = NEVER;
            }
            else Walk_timer -= diff;

            if (Dot_timer <= diff) // Unleash essence every 9 seconds cca
            {
                DoCast(SPELL_UNLEASH_ESSENCE);
                Dot_timer = urand(8000,10000);
            }
            else Dot_timer -= diff;

            if (me->HasReactState(REACT_AGGRESSIVE))
                DoMeleeAttackIfReady();
        }
    };
};

class spell_gen_wrack : public SpellScriptLoader
{
public:
    spell_gen_wrack() : SpellScriptLoader("spell_gen_wrack") { }

    class spell_gen_wrack_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_wrack_AuraScript);

        void HandleTick(AuraEffect const* aurEff)
        {
            Unit* target = GetTarget();

            if (target && target->ToPlayer())
            {
                uint32 result = 2;

                for(uint32 i = 0; i < aurEff->GetTickNumber() - 1; i++ )
                    result *= 2;

                    const_cast<AuraEffect*>(aurEff)->SetAmount((int32)(result * 1000));
            }
        }

        void OnDispel(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {

            if(GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_STACK) // Anti  "Recursion" after recasting spell in AI
                return;

            if(GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            {
                Unit* caster = aurEff->GetCaster();
                if (!caster)
                    return;

                if (Creature * pSinestra = caster->ToCreature())
                {
                    pSinestra->AI()->DoAction(aurEff->GetBase()->GetDuration()); // Spreading of wrack is handling in Sinestra's AI
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_wrack_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_wrack_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_wrack_AuraScript();
    }
};

class IsNotTwilightFlame
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->ToCreature() && object->ToCreature()->GetEntry() == CREATURE_TWILIGHT_FLAMES)
                    return false;
            return true;
        }
};

class spell_gen_call_flames : public SpellScriptLoader
{
    public:
        spell_gen_call_flames() : SpellScriptLoader("spell_gen_call_flames") { }

        class spell_gen_call_flames_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_call_flames_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsNotTwilightFlame());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_call_flames_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC); // 7
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_call_flames_SpellScript();
        }
};


class spell_twilight_slicer : public SpellScriptLoader
{
    public:
        spell_twilight_slicer() : SpellScriptLoader("spell_twilight_slicer") {}

        class spell_twilight_slicerSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_twilight_slicerSpellScript)

            void FilterTargets(std::list<Unit*>& unitList)
            {
                if (!GetCaster() || !GetCaster()->ToCreature())
                    return;

                Creature* orb = GetClosestCreatureWithEntry(GetCaster(), CREATURE_SHADOW_ORB2, 200.0f);
                if (!orb)
                    return;

                for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                {
                    if ((*itr)->IsInBetween(GetCaster(),orb,orb->GetObjectSize()) == false)
                        itr = unitList.erase(itr);
                    else
                        ++itr;
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_twilight_slicerSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);

            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_twilight_slicerSpellScript();
        }
};

// SPITECALLERS'S INDOMITABLE
class spell_gen_indomitable : public SpellScriptLoader
{
public:
    spell_gen_indomitable() : SpellScriptLoader("spell_gen_indomitable") { }

    class spell_gen_indomitable_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_indomitable_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(!GetCaster() || !GetCaster()->ToCreature())
                return;

            GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, true);
            GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
            GetCaster()->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
        }

        void OnDispel(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(!GetOwner() || !GetOwner()->ToCreature())
                return;

            if(GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_ENEMY_SPELL)
            {
                GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK, false);
                GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                GetCaster()->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
                GetCaster()->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, false);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_gen_indomitable_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY_MASK, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_indomitable_AuraScript::OnDispel, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY_MASK, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_indomitable_AuraScript();
    }
};

//Twilight Carapace
class spell_gen_carapace : public SpellScriptLoader
{
    public: spell_gen_carapace() : SpellScriptLoader("spell_gen_carapace") { }

        class spell_gen_carapace_AuraScript : public AuraScript 
        {
            PrepareAuraScript(spell_gen_carapace_AuraScript);

            void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = -1;
            }

            void Absorb(AuraEffect * /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                absorbAmount = dmgInfo.GetDamage() + 1; // +1 for sure
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_carapace_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_gen_carapace_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_gen_carapace_AuraScript();
        }
};

void AddSC_sinestra()
{
    new boss_sinestra();
    new mob_calen();
    new mob_shadow_orb_I();
    new mob_shadow_orb_II();
    new mob_Twilight_Drake();
    new twilight_whelp();
    new mob_Twilight_flames();
    new Twilight_egg();
    new Whelp_puddle();
    new mob_Twilight_spitecaller();

    new spell_gen_wrack();
    new spell_gen_indomitable();
    new spell_gen_carapace();
    new spell_gen_call_flames();
    new spell_twilight_slicer();
}

/*

delete from spell_script_names where spell_id in (95855,92852,92954,89421,92955,89435,92956,90045,87654)

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (95855, 'spell_gen_call_flames');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (92852, 'spell_twilight_slicer');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (92954, 'spell_twilight_slicer');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (89421, 'spell_gen_wrack');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (92955, 'spell_gen_wrack');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (89435, 'spell_gen_wrack');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (92956, 'spell_gen_wrack');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (90045, 'spell_gen_indomitable');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (87654, 'spell_gen_carapace');

INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (407189, 0, 0, 0, 0, 0, 11686, 0, 0, 0, 'Twilight_flames', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 1, 2, 2, 0, 24, 7.5, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 10, 1048576, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1, 3, 11.1289, 1, 0.9641, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'mob_Twilight_flames', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (45213, 0, 0, 0, 0, 0, 34335, 0, 0, 0, 'Sinestra', 'Consort of Deathwing', '', 0, 88, 88, 3, 14, 14, 0, 1, 1.14286, 1, 1, 90, 120, 0, 120, 1000, 2000, 0, 2, 4, 0, 0, 0, 0, 0, 0, 477.6, 682.8, 159, 2, 108, 45213, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2000, 2500, '', 0, 4, 319.53, 400, 1.0307, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2145370367, 0, 'boss_sinestra', 14480);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (46277, 0, 0, 0, 0, 0, 34541, 0, 0, 0, 'Calen', '', '', 0, 88, 88, 3, 35, 35, 0, 1, 1.14286, 1, 1, 530, 713, 0, 827, 7.5, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 399, 559, 169, 2, 4100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 49.4, 214.823, 1, 0, 0, 0, 0, 0, 0, 0, 171, 1, 0, 0, 0, 'mob_calen', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (46842, 0, 0, 0, 0, 0, 35318, 0, 0, 0, 'Pulsing Twilight Egg', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 1, 2, 2, 0, 24, 7.5, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1, 3, 11.18, 1, 0.9641, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'Twilight_egg', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (47058, 0, 0, 0, 0, 0, 35408, 0, 0, 0, 'Meteor', '', 'openhand', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 0, 2, 2, 0, 24, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 10, 1610613760, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 10, 1, 1, 0, 0, 0, 0, 0, 0, 0, 98, 1, 0, 0, 0, 'Whelp_puddle', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (47265, 0, 0, 0, 0, 0, 19295, 0, 0, 0, 'Twilight Whelp', '', '', 0, 86, 86, 3, 14, 14, 0, 1, 1.14286, 1, 1, 500, 700, 0, 686, 10, 2000, 0, 1, 0, 0, 0, 0, 0, 0, 0, 411, 568, 173, 2, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1, 4, 2.157, 1, 0.9641, 0, 0, 0, 0, 0, 0, 0, 150, 1, 0, 0, 0, 'twilight_whelp', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (48415, 0, 0, 0, 0, 0, 36137, 0, 0, 0, 'Twilight Spitecaller', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 0, 450, 650, 0, 629, 100, 2000, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1, 3, 7.42, 40, 0.9641, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'mob_Twilight_spitecaller', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (48436, 0, 0, 0, 0, 0, 32950, 0, 0, 0, 'Twilight Drake', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 1, 400, 500, 0, 514, 100, 2000, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 1, 4, 11.09, 1, 0.9641, 0, 0, 0, 0, 0, 0, 0, 154, 1, 0, 0, 0, 'mob_Twilight_Drake', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (49862, 0, 0, 0, 0, 0, 18996, 0, 0, 0, 'Shadow Orb', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 0, 2, 2, 0, 24, 1, 0, 0, 1, 33554432, 0, 0, 0, 0, 0, 0, 1, 1, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 10, 1, 1, 0, 0, 0, 0, 0, 0, 0, 106, 1, 0, 0, 0, 'mob_shadow_orb_I', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (49863, 0, 0, 0, 0, 0, 18996, 0, 0, 0, 'Shadow Orb', '', '', 0, 87, 87, 3, 14, 14, 0, 1, 1.14286, 1, 0, 2, 2, 0, 24, 1, 0, 0, 1, 33554432, 0, 0, 0, 0, 0, 0, 1, 1, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 10, 1, 1, 0, 0, 0, 0, 0, 0, 0, 106, 1, 0, 0, 0, 'mob_shadow_orb_II', 1);
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (51608, 0, 0, 0, 0, 0, 11686, 0, 0, 0, 'Barrier Cosmetic Stalker (DND)', '', '', 0, 87, 87, 3, 35, 35, 0, 1, 1.14286, 2, 1, 2, 2, 0, 24, 7.5, 0, 0, 1, 33554432, 0, 0, 0, 0, 0, 0, 1, 1, 0, 10, 1048576, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 500.1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, '', 1);

UPDATE `gameobject_template` SET `displayId`=10317 WHERE `entry`=208045 LIMIT 1;

UPDATE `creature_template` SET `mechanic_immune_mask`=2145370111 WHERE  `entry`=45213 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=2145370111 WHERE  `entry`=46842 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=2145370111 WHERE  `entry`=47265 LIMIT 1;
UPDATE `creature_template` SET `mechanic_immune_mask`=2145370111 WHERE  `entry`=48436 LIMIT 1;
*/
