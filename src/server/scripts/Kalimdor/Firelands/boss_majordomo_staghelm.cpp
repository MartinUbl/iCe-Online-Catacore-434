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
#include "firelands.h"


enum Spells
{
    // Forms
    SPELL_CAT_FORM              = 98374,
    SPELL_SCORPION_FORM         = 98379,

    // Buffs
    SPELL_ADRENALINE            = 97238,
    SPELL_BERSERK               = 26662,
    SPELL_FURY                  = 97235,

    // Scorpion form
    SPELL_FLAME_SCYTE           = 98474,

    // Cat form
    SPELL_LEAPING_FLAMES        = 98476,
    SPELL_LEAPING_FLAMES_AOE    = 98535,
    SPELL_SUMMON_SPIRIT         = 101222,

    // Transition
    SPELL_FIERY_CYCLONE         = 98443,

    // Seeds
    SPELL_SEARING_SEEDS         = 98450,
    SPELL_SEARING_SEED_DMG      = 98620,

    // Burning orbs
    SPELL_BURNING_ORBS          = 98451, // 4 s dummy cast
    SPELL_SUMMON_BURNING_ORB    = 98565, // summon one orb at random location
    SPELL_BURNING_ORB_VISUAL    = 98583, // need to turn off triggering spell on effect 0
    SPELL_BURNING_ORB_DAMAGE    = 98584, // "beam"

    // Heroic spells
    SPELL_UNCOMMON_CONCENTRATION        = 98254, // 25 %
    SPELL_RARE_CONCENTRATION            = 98253, // 50 %
    SPELL_EPIC_CONCENTRATION            = 98252, // 75 %
    SPELL_LEGENDARY_CONCENTRATION       = 98245, // 100 %

    SPELL_CONCENTRATION_BAR             = 98229
};

enum MajordomoPhase
{
    PHASE_DRUID,
    PHASE_CAT,
    PHASE_SCORPION
};

enum Creatures
{
    MAJORDOMO_STAGHELM      = 52571,
    SPIRIT_OF_FLAME         = 52593,
    BURNING_ORB             = 53216,
    FANDRAL_FLAME           = 53696
};


struct Yells
{
    uint32 sound;
    const char * text;
};

static const Yells RandomKill[4]=
{
    {24477, "Burn."},
    {24479, "Soon, ALL of Azeroth will burn!"},
    {24480, "So much power!"},
    {24481, "You stood in the fire!"},
};

uint32 energyField[10] = {18,12,10,8,7,6,5,4,3,2}; // Very wierd calculation of time, so i do it manually for sure

#define MINUTE 60000
#define NEVER  (0xffffffff) // used as "delayed" timer ( max uint32 value)

#define MIDDLE_X  430.0f
#define MIDDLE_Y -63.0f
#define MIDDLE_Z  79.0f

#define FIREWALL_MAJORDOMO  208873

class boss_majordomo_staghelm : public CreatureScript
{
public:
    boss_majordomo_staghelm() : CreatureScript("boss_majordomo_staghelm") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_majordomo_staghelmAI (creature);
    }

    struct boss_majordomo_staghelmAI : public ScriptedAI
    {
        boss_majordomo_staghelmAI(Creature* creature) : ScriptedAI(creature),Summons(creature)
        {
            instance = creature->GetInstanceScript();
            //me->SummonGameObject(FIREWALL_MAJORDOMO,576.04f,-61.8f,90.34f,5.6f,0,0,0,0,0); Temporary disabled

            // Summon 2 orbs for achievement (Only the Penitent...)
            me->SummonCreature(FANDRAL_FLAME, 508.0f, -27.0f, 84.0f, 2.83f);
            me->SummonCreature(FANDRAL_FLAME, 510.0f, -97.0f, 84.0f, 2.83f);
        }

        InstanceScript* instance;
        uint8 PHASE;
        uint32 morphs;
        uint32 Morph_timer;
        uint32 Energy_timer;
        uint32 Human_timer;
        uint32 Berserk_timer;
        uint32 Phase_check_timer;
        uint32 eventTimer;
        uint32 powerTimer;
        uint32 energyCounter;
        bool FromCatToScorpion;
        bool barUsed;
        bool canMorph;

        SummonList Summons;

        void Reset()
        {
            if(instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetData(TYPE_STAGHELM, NOT_STARTED);
            }

            canMorph = true; // First transformation
            energyCounter = 0;
            TransformToDruid();
            morphs = 0;
            PHASE = PHASE_DRUID;
            eventTimer = 1000;
            Phase_check_timer = 2000;
            powerTimer = 2500;
            Morph_timer = 2000;
            Energy_timer = 1000;
            Berserk_timer = 10 * MINUTE;
            Human_timer = NEVER;
            barUsed = false;
        }

        void KilledUnit(Unit * victim)
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint8 rand_ = urand(0, 3);
                PlayAndYell(RandomKill[rand_].sound, RandomKill[rand_].text);
            }
        }

        void JustDied(Unit * /*victim*/)
        {
            me->RemoveAurasDueToSpell(SPELL_CONCENTRATION_BAR);

            if(instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetData(TYPE_STAGHELM, DONE);

                if (IsHeroic())
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CONCENTRATION_BAR);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNCOMMON_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RARE_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_EPIC_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LEGENDARY_CONCENTRATION);
                }
            }

            Summons.DespawnAll();
            me->MonsterYell("My studies... had only just begun...", LANG_UNIVERSAL, 0);
            me->PlayDistanceSound(24464);

            GameObject * wall = me->FindNearestGameObject(FIREWALL_MAJORDOMO,500.0f);

            if (wall)
                wall->Delete();
        }

        void EnterCombat(Unit* who)
        {
            std::list<Creature*> orbs;
            GetCreatureListWithEntryInGrid(orbs, me, FANDRAL_FLAME, 200.0f);

            for (std::list<Creature*>::iterator iter = orbs.begin(); iter != orbs.end(); ++iter)
                (*iter)->ForcedDespawn();

             me->SetWalk(false);

            if(instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                instance->SetData(TYPE_STAGHELM, IN_PROGRESS);
            }

            morphs++;
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,10.0f);
            me->SetInCombatWithZone();

            me->SetFloatValue(UNIT_FIELD_COMBATREACH,5.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,10.0f);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType typeOfDamage)
        {
            if (victim && victim->ToPlayer() && IsHeroic())
            {
                victim->RemoveAura(SPELL_UNCOMMON_CONCENTRATION);
                victim->RemoveAura(SPELL_RARE_CONCENTRATION);
                victim->RemoveAura(SPELL_EPIC_CONCENTRATION);
                victim->RemoveAura(SPELL_LEGENDARY_CONCENTRATION);
                victim->SetPower(POWER_SCRIPTED,0);
                victim->RemoveAura(SPELL_CONCENTRATION_BAR);  // client don't want visualy reset power bar :)
                victim->AddAura(SPELL_CONCENTRATION_BAR,victim);
            }
        }

        void EnterEvadeMode()
        {
            me->RemoveAurasDueToSpell(SPELL_CONCENTRATION_BAR);
            me->SetHomePosition(516.0f,-62.0f,85.0f,M_PI);

            if(instance)
            {
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetData(TYPE_STAGHELM, NOT_STARTED);

                if (IsHeroic())
                {
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_CONCENTRATION_BAR);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNCOMMON_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_RARE_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_EPIC_CONCENTRATION);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LEGENDARY_CONCENTRATION);
                }
            }

            Summons.DespawnAll();
            TransformToDruid();
            ScriptedAI::EnterEvadeMode();
            me->RemoveAllAuras();

            me->SetFloatValue(UNIT_FIELD_COMBATREACH,5.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,20.0f);
        }

        void JustSummoned(Creature* summon)
        {
            Summons.push_back(summon->GetGUID());
        }

        void PlayAndYell(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me,soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void TransformToCat()
        {
            canMorph = false;

            if (barUsed == false && IsHeroic())
            {
                me->CastSpell(me, SPELL_CONCENTRATION_BAR, true);

                if (instance)
                {
                    instance->DoSetMaxScriptedPowerToPlayers(100);
                    instance->DoSetScriptedPowerToPlayers(0);
                }

                barUsed = true;
            }
            morphs++;
            me->RemoveAura(SPELL_ADRENALINE);
            me->RemoveAura(SPELL_SCORPION_FORM);
            me->CastSpell(me,SPELL_CAT_FORM,false);

            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY,100);
            me->SetPower(POWER_ENERGY,0);

            me->CastSpell(me,SPELL_FURY,false);
            PHASE = PHASE_CAT;
            me->MonsterYell("Behold the rage of the Firelands!", LANG_UNIVERSAL, 0);
            me->PlayDistanceSound(24485);
        }

        void TransformToScorpion()
        {
            canMorph = false;

            if (barUsed == false && IsHeroic())
            {
                me->CastSpell(me, SPELL_CONCENTRATION_BAR, true);

                if (instance)
                {
                    instance->DoSetMaxScriptedPowerToPlayers(100);
                    instance->DoSetScriptedPowerToPlayers(0);
                }

                barUsed = true;
            }

            morphs++;
            me->RemoveAura(SPELL_ADRENALINE);
            me->RemoveAura(SPELL_CAT_FORM);

            me->CastSpell(me,SPELL_SCORPION_FORM,false);

            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY,100);
            me->SetPower(POWER_ENERGY,0);

            me->CastSpell(me,SPELL_FURY,false);

            PHASE = PHASE_SCORPION;
            me->MonsterYell("The master's power takes on many forms...", LANG_UNIVERSAL, 0);
            me->PlayDistanceSound(24483);
        }

        void TransformToDruid()
        {
            canMorph = true;

            me->RemoveAura(SPELL_ADRENALINE);
            me->RemoveAura(SPELL_CAT_FORM);
            me->RemoveAura(SPELL_SCORPION_FORM);

            me->setPowerType(POWER_ENERGY);
            me->SetMaxPower(POWER_ENERGY,100);
            me->SetPower(POWER_ENERGY,0);
            PHASE = PHASE_DRUID;
        }

        bool IsRangedClass(Player * p )
        {
            return(p->getClass() == CLASS_WARLOCK || p->getClass() == CLASS_MAGE || p->getClass() == CLASS_PRIEST ||  p->getClass() ==CLASS_HUNTER 
            || p->GetActiveTalentBranchSpec() == SPEC_SHAMAN_ELEMENTAL || p->GetActiveTalentBranchSpec() == SPEC_SHAMAN_RESTORATION 
            ||  p->GetActiveTalentBranchSpec() == SPEC_DRUID_RESTORATION || p->GetActiveTalentBranchSpec() == SPEC_DRUID_BALANCE );
        }

        bool IsInHumanForm(void)
        {
            return (me->HasAura(SPELL_SCORPION_FORM) || me->HasAura(SPELL_CAT_FORM)) ? false : true;
        }

        Player * SelectRandomRangedPlayer() // Target for leaping flames need to be ranged class
        {
            std::list<Player*> ranged_targets;

            std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
            for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
            {
                Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());

                if(unit && unit->IsAlive() && unit->ToPlayer() && IsRangedClass(unit->ToPlayer()) )
                    ranged_targets.push_back(unit->ToPlayer());
            }

            if (!ranged_targets.empty())
            {
                std::list<Player*>::const_iterator j = ranged_targets.begin();
                advance(j, rand()%ranged_targets.size());
                return (*j);
            }

            return NULL;
        }


        void SetDurationForSeeds()
        {
            uint32 counter = 0;
            uint32 time_gap = (Is25ManRaid()) ? (55/23) : (55/9); // (55 seconds / number of players ) --> tanks are excluded
            time_gap *= 1000; // Need miliseconds

            std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
            for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
            {
                Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());

                if(unit && unit->ToPlayer())
                {
                    Aura * seed_aura = unit->GetAura(SPELL_SEARING_SEEDS);

                    if(seed_aura)
                    {
                        if (unit == me->GetVictim()) // Don't apply seed on main tank
                        {
                            seed_aura->Remove(AURA_REMOVE_BY_DEFAULT);
                            continue;
                        }
                        seed_aura->SetDuration(10000 + (counter * time_gap));
                        counter++;
                    }
                }
            }
        }

        void SpawnBurningOrb(void)
        {
            float angle = (float)urand(0,(2*M_PI)*10);
            angle /= 10.0f;
            float length= urand(25,45);
            me->SummonCreature(BURNING_ORB,MIDDLE_X + cos(angle)*length,MIDDLE_Y + sin(angle)* length,MIDDLE_Z,angle,TEMPSUMMON_CORPSE_DESPAWN,0);
        }

        // If >= 7/18 (10 man / 25 man) people are clustered together, switch to Scorpion form 
        // This algorithm has time complexity O(n^2)
        // TODO : Maybe find better algorithm
        bool RaidIsClusteredTogether(void) 
        {
            uint32 counter = 0;
            uint32 maximum = (Is25ManRaid()) ? 18 : 7;

            Map * map = me->GetMap();
            if (!map)
                return false;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return false;

            std::list<Player*> players;
            // Copy players to secons list
            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                if(Player* pl = itr->getSource())
                    if (pl->IsInWorld() && pl->IsAlive() && !pl->IsGameMaster())
                        players.push_back(itr->getSource());

            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if(Player* p = itr->getSource())
                {
                    counter = 0;

                    if (p->IsInWorld() && p->IsAlive() && !p->IsGameMaster())
                    {
                        // Measure distance for player to every player
                        for (std::list<Player*>::iterator j = players.begin(); j != players.end(); ++j)
                        {
                            if ((*j) && (*j)->IsInWorld())
                            {
                                if ((*j) == p) // Exclude self
                                    continue;
                                if (p->GetExactDist2d(*j) <= 15.0f)
                                    counter++;
                            }
                        }
                        if (counter >= maximum) // Stop looking
                            return true;
                    }
                }
            }
            return false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim() || !me->GetVictim())
            {
                if (eventTimer <= diff)
                {
                    if (me->FindNearestCreature(53619,250.0f,true))
                    {
                        me->SetReactState(REACT_PASSIVE);
                        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                    }
                    else
                    {
                        if (!me->isMoving())
                        {
                            me->MonsterYell("Very well. Witness the raw power of my new lord!", LANG_UNIVERSAL, 0);
                            me->PlayDistanceSound(24464);

                            me->SetWalk(true);
                            me->GetMotionMaster()->MovePoint(0,516.0f,-62.0f,85.0f);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                        }
                        eventTimer = NEVER;
                        return;
                    }

                    eventTimer = 1000;
                }
                else eventTimer -= diff;

                return;
            }

            if (me->GetPositionX() < 352.0f)
                ScriptedAI::EnterEvadeMode();

            if (!me->IsWithinLOSInMap(me->GetVictim()) && IsInHumanForm() == false)
                me->Kill(me->GetVictim());

            if (powerTimer <= diff && IsHeroic())
            {
                if (!instance)
                    return;

                Map::PlayerList const& plList = instance->instance->GetPlayers();

                if (plList.isEmpty())
                    return;

                for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    if ( Player * p = itr->getSource())
                    {
                        if (p->IsAlive() && !p->IsGameMaster() && p->IsInWorld())
                        {
                            uint32 power = p->GetPower(POWER_SCRIPTED);

                            if (power + 5 > 100)
                                continue;

                            power +=5;
                            p->SetPower(POWER_SCRIPTED,power);

                            switch (power)
                            {
                                case 25:
                                {
                                    if (!p->HasAura(SPELL_UNCOMMON_CONCENTRATION))
                                        p->CastSpell(p,SPELL_UNCOMMON_CONCENTRATION,true);
                                    break;
                                }
                                case 50:
                                {
                                    if (!p->HasAura(SPELL_RARE_CONCENTRATION))
                                    {
                                        p->CastSpell(p,SPELL_RARE_CONCENTRATION,true);
                                        p->RemoveAurasDueToSpell(SPELL_UNCOMMON_CONCENTRATION);
                                    }
                                    break;
                                }
                                case 75:
                                {
                                    if (!p->HasAura(SPELL_EPIC_CONCENTRATION))
                                    {
                                        p->CastSpell(p,SPELL_EPIC_CONCENTRATION,true);
                                        p->RemoveAurasDueToSpell(SPELL_RARE_CONCENTRATION);
                                    }
                                    break;
                                }
                                case 100:
                                {
                                    if (!p->HasAura(SPELL_LEGENDARY_CONCENTRATION))
                                    {
                                        p->CastSpell(p,SPELL_LEGENDARY_CONCENTRATION,true);
                                        p->RemoveAurasDueToSpell(SPELL_EPIC_CONCENTRATION);
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
                powerTimer = 1000;
            }
            else powerTimer -= diff;

            if(Phase_check_timer <= diff)
            {
                if(morphs == 3)
                {
                    bool clustered = RaidIsClusteredTogether();

                    if(clustered == true  && me->HasAura(SPELL_SCORPION_FORM))
                    {
                        Phase_check_timer = 2000;
                        return;
                    }

                    if (clustered == false && me->HasAura(SPELL_CAT_FORM))
                    {
                        Phase_check_timer = 2000;
                        return;
                    }

                    FromCatToScorpion = me->HasAura(SPELL_CAT_FORM);
                    TransformToDruid();
                    me->CastSpell(me,SPELL_FIERY_CYCLONE,true);

                    if(FromCatToScorpion) // if we are transforming from cat to scorpion
                    {
                        me->CastSpell(me,SPELL_SEARING_SEEDS,false); // cast searing seeds
                        me->MonsterYell("Blaze of Glory!", LANG_UNIVERSAL, 0);
                        me->PlayDistanceSound(24472);
                    }
                    else                  // if we are transforming from scorpion to cat
                    {
                        me->CastSpell(me,SPELL_BURNING_ORBS,false); // cast Burning orbs
                        me->MonsterYell("Nothing but ash!", LANG_UNIVERSAL, 0);
                        me->PlayDistanceSound(24478);
                    }

                    Human_timer = 4050;
                    Phase_check_timer = 4050;
                    morphs = 0;
                    return;
                }

                if (RaidIsClusteredTogether())
                {
                    if (!me->HasAura(SPELL_SCORPION_FORM) && canMorph)
                        TransformToScorpion();
                }
                else
                {
                    if (!me->HasAura(SPELL_CAT_FORM) && canMorph)
                        TransformToCat();
                }

                Phase_check_timer = 2000;
            }
            else Phase_check_timer -= diff;

            if(Human_timer <= diff)
            {
                if(FromCatToScorpion)
                    SetDurationForSeeds();
                else
                {
                    uint8 max = (Is25ManRaid()) ? 5 : 2;

                    for(uint8 i = 0; i < max ; i++)
                        //me->CastSpell(me,SPELL_SUMMON_BURNING_ORB,true)
                        SpawnBurningOrb(); // For safety I spawn orbs manually
                }

                Human_timer = NEVER;
            }
            else Human_timer -= diff;

            if(Energy_timer <= diff) // Manualy filling energy bar
            {
                uint32 adrenalineStacks = me->GetAuraCount(SPELL_ADRENALINE);

                if(me->getPowerType() == POWER_ENERGY && !IsInHumanForm())
                {
                    adrenalineStacks = (adrenalineStacks > 9) ? 9 : adrenalineStacks; // Out of bounds protection
                    me->SetPower(POWER_ENERGY,me->GetPower(POWER_ENERGY) + (100 / energyField[adrenalineStacks]));
                }
                Energy_timer = 1000;
            }
            else Energy_timer -= diff;

            if(Berserk_timer <= diff) // Berserk after 10 minutes
            {
                me->CastSpell(me,SPELL_BERSERK,true);
                Berserk_timer = NEVER;
            }
            else Berserk_timer -= diff;


            if( PHASE == PHASE_SCORPION && me->GetPower(POWER_ENERGY) == 100)
            {
                me->CastSpell(me,SPELL_FLAME_SCYTE,false);
                canMorph = true;
            }

            if( PHASE == PHASE_CAT && me->GetPower(POWER_ENERGY) == 100)
            {
                canMorph = true;

                //me->CastSpell(me,SPELL_SUMMON_SPIRIT,false); // Summon 1 cat copy of Majordomo
                me->SummonCreature(52593, me->GetPositionX(), me->GetPositionY(),me->GetPositionZ(),me->GetOrientation(),TEMPSUMMON_CORPSE_DESPAWN, 500);
                Player * p = SelectRandomRangedPlayer();

                if(p == NULL || p->IsInWorld() == false) // We didnt find valid ranged target - > jump to random player
                {
                    if ( Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                    {
                        me->CastSpell(player,SPELL_LEAPING_FLAMES,false);
                        me->CastSpell(player->GetPositionX(),player->GetPositionY(),player->GetPositionZ(),SPELL_LEAPING_FLAMES_AOE,false);
                    }
                }
                else // We found valid ranged target
                {
                    me->CastSpell(p,SPELL_LEAPING_FLAMES,false);
                    me->CastSpell(p->GetPositionX(),p->GetPositionY(),p->GetPositionZ(),SPELL_LEAPING_FLAMES_AOE,false);
                }

            }

            DoMeleeAttackIfReady();
        }
    };

};

class spirit_of_flame_npc : public CreatureScript
{
public:
   spirit_of_flame_npc() : CreatureScript("spirit_of_flame_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new spirit_of_flame_npcAI (creature);
    }

    struct spirit_of_flame_npcAI : public ScriptedAI
    {
        spirit_of_flame_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            instance = me->GetInstanceScript();
        }

        InstanceScript * instance;

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();

            if ( Creature* pMajor = me->FindNearestCreature(52571,100.0f,true) )
            {
                if (pMajor->GetVictim())
                {
                    me->AddThreat(pMajor->GetVictim(),50.0f);
                    me->GetMotionMaster()->MoveChase(pMajor->GetVictim());
                }
            }
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType typeOfDamage)
        {
            if (victim && victim->ToPlayer() && IsHeroic())
            {
                victim->RemoveAura(SPELL_UNCOMMON_CONCENTRATION);
                victim->RemoveAura(SPELL_RARE_CONCENTRATION);
                victim->RemoveAura(SPELL_EPIC_CONCENTRATION);
                victim->RemoveAura(SPELL_LEGENDARY_CONCENTRATION);
                victim->SetPower(POWER_SCRIPTED,0);
                victim->RemoveAura(SPELL_CONCENTRATION_BAR); // client don't want visualy reset power bar :)
                victim->AddAura(SPELL_CONCENTRATION_BAR,victim);
            }
        }

        void EnterCombat(Unit* /*target*/)
        {
            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void UpdateAI (const uint32 diff)
        {
            DoMeleeAttackIfReady();
        }
    };
};

class burning_orb_npc : public CreatureScript
{
public:
   burning_orb_npc() : CreatureScript("burning_orb_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new burning_orb_npcAI (creature);
    }

    struct burning_orb_npcAI : public ScriptedAI
    {
        burning_orb_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
        }

        uint32 Beam_timer;

        void Reset()
        {
            Beam_timer = 1000;
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();
            me->CastSpell(me,SPELL_BURNING_ORB_VISUAL,true);
            me->ForcedDespawn(70000);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType typeOfDamage)
        {
            if (victim && victim->ToPlayer() && IsHeroic())
            {
                victim->RemoveAura(SPELL_UNCOMMON_CONCENTRATION);
                victim->RemoveAura(SPELL_RARE_CONCENTRATION);
                victim->RemoveAura(SPELL_EPIC_CONCENTRATION);
                victim->RemoveAura(SPELL_LEGENDARY_CONCENTRATION);
                victim->SetPower(POWER_SCRIPTED,0);
                victim->RemoveAura(SPELL_CONCENTRATION_BAR); // client don't want visualy reset power bar :)
                victim->AddAura(SPELL_CONCENTRATION_BAR,victim);
            }
        }

        void UpdateAI (const uint32 diff)
        {
            if(Beam_timer <= diff)
            {
                if ( Unit* player = SelectTarget(SELECT_TARGET_NEAREST, 0, 200.0f, true) )
                    me->CastSpell(player,SPELL_BURNING_ORB_DAMAGE,true);

                Beam_timer = 2100;
            }
            else Beam_timer -= diff;
        }
    };
};

class staghelm_flame_orb : public CreatureScript
{
public:
   staghelm_flame_orb() : CreatureScript("staghelm_flame_orb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new staghelm_flame_orbAI (creature);
    }

    struct staghelm_flame_orbAI : public ScriptedAI
    {
        staghelm_flame_orbAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,/*|UNIT_FLAG_NON_ATTACKABLE|*/UNIT_FLAG_DISABLE_MOVE);
            if (me->GetPositionY() < -30.0f)
                leftOrb = false;
            else
                leftOrb = true;
        }

        uint32 checkTimer;
        bool changed;
        bool leftOrb;

        void Reset()
        {
            checkTimer = NEVER;
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me,SPELL_BURNING_ORB_VISUAL,true);
        }

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);

            if (spell->Id != 45576) // Beam
                return;

            if (me->GetAuraCount(45576) > 1) // Beam
                return;

            checkTimer = 3000;
        }

        void UpdateAI (const uint32 diff)
        {
            if (checkTimer <= diff && leftOrb)
            {
                Creature *rightOrb = NULL;
                std::list<Creature*> orbs;
                GetCreatureListWithEntryInGrid(orbs, me, FANDRAL_FLAME, 200.0f);

                for (std::list<Creature*>::iterator iter = orbs.begin(); iter != orbs.end(); ++iter)
                {
                    if ((*iter)->GetPositionY() < -30.0f)
                    {
                        rightOrb = *iter;
                    }
                }

                if (rightOrb == NULL)
                {
                    me->ForcedDespawn();
                    return;
                }

                if (me->GetAuraCount(45576) >= 3 && rightOrb->GetAuraCount(45576) >= 3)
                {
                    me->RemoveAllAuras();
                    rightOrb->RemoveAllAuras();

                    me->ForcedDespawn(500);
                    rightOrb->ForcedDespawn(500);

                    if (InstanceScript * instance = me->GetInstanceScript())
                        instance->DoCompleteAchievement(5799); // Only the Penitent... achievement
                }
                checkTimer = NEVER;
            }
            else checkTimer -= diff;

        }
    };
};

enum DruidsSpells
{
    RECKLESS_LEAP           = 99629,
    RECKLESS_LEAP_STUN      = 99646,
    REACTIVE_FLAMES         = 99649,
    KNEEEL_TO_THE_FLAME     = 99705,
    SUNFIRE                 = 99626,
    FIRE_CAT_TRANSFORM      = 99574
};

class druid_of_the_flame : public CreatureScript
{
public:
   druid_of_the_flame() : CreatureScript("druid_of_the_flame") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new druid_of_the_flameAI (creature);
    }

    struct druid_of_the_flameAI : public ScriptedAI
    {
        druid_of_the_flameAI(Creature* creature) : ScriptedAI(creature) 
        {
            instance = me->GetInstanceScript();
            int32 x = me->GetPositionX();
            int32 y = me->GetPositionY();

            isKitty = (x == 516 && y == -62) ? false : true;

            if (isKitty)
            {
                me->CastSpell(me,REACTIVE_FLAMES,true);
                me->CastSpell(me,FIRE_CAT_TRANSFORM,true);
            }
        }

        uint32 jumpTimer;
        uint32 StunTimer;
        uint32 kneelTimer;
        uint32 sunfireTimer;
        bool isKitty;
        InstanceScript * instance;

        void Reset()
        {
            if (isKitty == false)
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            sunfireTimer = 2000;
            kneelTimer = 1000;
            jumpTimer = 2000;
            StunTimer = NEVER;
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
        {
            if (spell->Id == KNEEEL_TO_THE_FLAME)
            {
                std::list<Creature*> orbs;
                GetCreatureListWithEntryInGrid(orbs, me, FANDRAL_FLAME, 200.0f);

                for (std::list<Creature*>::iterator iter = orbs.begin(); iter != orbs.end(); ++iter)
                    (*iter)->ForcedDespawn();
            }
        }

        void EnterEvadeMode()
        {
            ScriptedAI::EnterEvadeMode();
            if (isKitty)
            {
                me->CastSpell(me,REACTIVE_FLAMES,true);
                me->CastSpell(me,FIRE_CAT_TRANSFORM,true);
            }
        }

        void EnterCombat(Unit* /*target*/)
        {
            me->SetInCombatWithZone();

            if(instance)
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            std::list<Creature*> orbs;
            GetCreatureListWithEntryInGrid(orbs, me, FANDRAL_FLAME, 200.0f);

            for (std::list<Creature*>::iterator iter = orbs.begin(); iter != orbs.end(); ++iter)
                (*iter)->ForcedDespawn();
        }

        void JustReachedHome()
        {
            if (isKitty)
            {
                me->CastSpell(me,REACTIVE_FLAMES,true);
                me->CastSpell(me,FIRE_CAT_TRANSFORM,true);
            }

            std::list<Creature*> druids;
            GetCreatureListWithEntryInGrid(druids, me, 53619, 250.0f);

            if (druids.empty())
                return;

            for (std::list<Creature*>::iterator iter = druids.begin(); iter != druids.end(); ++iter)
            {
                if ((*iter)->isDead())
                {
                    (*iter)->Respawn();
                    (*iter)->GetMotionMaster()->MoveTargetedHome();
                }
            }
        }

        void UpdateAI (const uint32 diff)
        {
            if (!UpdateVictim())
            {
                if (isKitty == false)
                {
                    if(kneelTimer <= diff)
                    {
                        if (!me->isMoving())
                        {
                            me->MonsterYell("Kneel before the burning flame !",LANG_UNIVERSAL,0);
                            me->CastSpell(me,KNEEEL_TO_THE_FLAME,false);
                        }
                        kneelTimer = 6000;
                    }
                    else kneelTimer -= diff;
                }
                return;
            }

            if (isKitty)
            {
                if(jumpTimer <= diff)
                {
                    if (Unit* player = SelectTarget(SELECT_TARGET_NEAREST, 0, 100.0f, true) )
                    {
                        me->getThreatManager().resetAllAggro();
                        me->AddThreat(player,50000.0f);
                        me->CastSpell(player,RECKLESS_LEAP,false);
                    }

                    StunTimer = 5000;
                    jumpTimer = 15000;
                }
                else jumpTimer -= diff;

                if(StunTimer <= diff)
                {
                    me->CastSpell(me->GetVictim(),RECKLESS_LEAP_STUN,true);
                    StunTimer = NEVER;
                }
                else StunTimer -= diff;

            }
            else if (isKitty == false) // Druid in human form
            {
                if (sunfireTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCasted(false))
                        me->CastSpell(me,SUNFIRE,false);
                    sunfireTimer = 3000;
                }
                else sunfireTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class IsKneeling
{
    public:
        bool operator()(Unit* unit) const
        {
            if (unit && (unit->getStandState() == UNIT_STAND_STATE_KNEEL || unit->GetEntry() == FANDRAL_FLAME))
                return true;
            else
                return false;
        }
};

class spell_gen_kneel_to_the_flame : public SpellScriptLoader
{
    public:
        spell_gen_kneel_to_the_flame() : SpellScriptLoader("spell_gen_kneel_to_the_flame") { }

        class spell_gen_kneel_to_the_flame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_kneel_to_the_flame_SpellScript);

            void RemoveKneelingTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsKneeling());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_kneel_to_the_flame_SpellScript::RemoveKneelingTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC); // 15
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_kneel_to_the_flame_SpellScript();
        }
};

class spell_searing_seed_explosion : public SpellScriptLoader
{
    public:
        spell_searing_seed_explosion() : SpellScriptLoader("spell_searing_seed_explosion") { }

        class spell_searing_seed_explosion_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_searing_seed_explosion_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* owner = GetOwner()->ToPlayer();
                if (!owner)
                    return;

                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEFAULT)
                    owner->CastSpell(owner, SPELL_SEARING_SEED_DMG, true);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_searing_seed_explosion_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_searing_seed_explosion_AuraScript();
        }
};

class spell_gen_flame_scythe : public SpellScriptLoader
{
    public:
        spell_gen_flame_scythe() : SpellScriptLoader("spell_gen_flame_scythe") { }

        class spell_gen_flame_scythe_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_flame_scythe_SpellScript);

            bool Load()
            {
                targetCount = 0;
                hit_counter = 0;
                return true;
            }

            void HandleHit(SpellEffIndex /*effIndex*/)
            {
                hit_counter ++;

                if (hit_counter == targetCount)
                    HandleEnergize();

                Unit * caster = GetCaster();
                if(caster == NULL)
                    return;

                uint32 damage = (GetHitDamage() * 120) /100; // Pre nerfed value ( + 20 % cca )

                damage *= ((caster->GetAuraCount(SPELL_FURY) * 8) + 100); // 8 % per apllication of fury
                damage /= 100;
                SetHitDamage(damage);
            }

            void CountHitTargets(std::list<Unit*>& unitList)
            {
                for (std::list<Unit*>::iterator itr = unitList.begin() ; itr != unitList.end();)
                {
                    if ((*itr)->IsGuardian() || (*itr)->IsPet())
                    {
                        itr = unitList.erase(itr);
                    }
                    else
                        ++itr;
                }

                targetCount = unitList.size();
            }

            void HandleEnergize()
            {
                if(targetCount < 7) // Fandral gains energy when his Flame Scythe hits fewer than 7 targets. 
                {
                    if(GetCaster() && GetCaster()->ToCreature() &&  GetCaster()->getPowerType() == POWER_ENERGY )
                        GetCaster()->SetPower(POWER_ENERGY,GetCaster()->GetPower(POWER_ENERGY) + (7 - targetCount ) * 10 ); // 10 Energy  per every player below 7
                }
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_flame_scythe_SpellScript::CountHitTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_UNKNOWN);
                OnEffect += SpellEffectFn(spell_gen_flame_scythe_SpellScript::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                //AfterCast += SpellCastFn(spell_gen_flame_scythe_SpellScript::HandleEnergize); // TODO Beg Gregory for implementation of AfterCast Hooklist
            }

        private:
            int32 targetCount;
            int32 hit_counter;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_flame_scythe_SpellScript();
        }
};

class spell_gen_leaping_flames : public SpellScriptLoader
{
    public:
        spell_gen_leaping_flames() : SpellScriptLoader("spell_gen_leaping_flames") { }

        class spell_gen_leaping_flames_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_leaping_flames_AuraScript);

            void HandlePeriodicDamage(AuraEffect const* aurEff)
            {
                Unit* caster = GetCaster();
                int32 _periodicDmg = 0;

                if( caster == NULL)
                    return;

                _periodicDmg = aurEff->GetBaseAmount();

                _periodicDmg *= ((caster->GetAuraCount(SPELL_FURY) * 8) + 100); // 8 % per apllication of fury
                _periodicDmg /= 100;

                const_cast<AuraEffect*>(aurEff)->SetAmount(_periodicDmg);
            }

                void Register()
                {
                    OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_leaping_flames_AuraScript::HandlePeriodicDamage, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_gen_leaping_flames_AuraScript();
        }
};

void AddSC_boss_majordomo_staghelm()
{
    //NPC'S
    new boss_majordomo_staghelm();//        52571
    new spirit_of_flame_npc(); //           52593
    new burning_orb_npc(); //               53216
    new druid_of_the_flame();
    new staghelm_flame_orb(); //            53696

    // SPELLS
    new spell_searing_seed_explosion(); //  98620,100215,100216,100217
    new spell_gen_flame_scythe(); //        98474,100212,100213,100214
    new spell_gen_leaping_flames(); //      98535,100206,100207,100208
    new spell_gen_kneel_to_the_flame();//   99705,100101
}

/*
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53619, 53803, 0, 0, 0, 0, 38441, 38442, 0, 0, 'Druid of the Flame', '', '', 0, 85, 85, 3, 14, 14, 0, 1.14286, 1.6, 1, 0, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 7, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50000, 100000, '', 0, 3, 43.9, 1, 1, 0, 0, 0, 0, 0, 0, 0, 125, 1, 53619, 646920191, 0, 'druid_of_the_flame', 15595);
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53803, 0, 0, 0, 0, 0, 38441, 38442, 0, 0, 'Druid of the Flame (1)', '', '', 0, 85, 85, 3, 14, 14, 0, 1.14286, 1.6, 1, 0, 30000, 33000, 0, 308, 1, 2000, 2000, 1, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 7, 72, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50000, 100000, '', 0, 3, 154, 1, 1, 0, 0, 0, 0, 0, 0, 0, 125, 1, 53619, 0, 0, '', 15595);
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (52571, 53856, 53857, 53858, 0, 0, 37953, 0, 0, 0, 'Majordomo Staghelm', 'Archdruid of the Flame', '', 0, 88, 88, 3, 14, 14, 0, 2, 1.71429, 1, 3, 60000, 70000, 0, 308, 1, 1250, 2000, 4, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 7, 108, 52571, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 380, 1, 1, 0, 0, 0, 0, 0, 0, 0, 169, 1, 0, 646922239, 1, 'boss_majordomo_staghelm', 15595);
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53856, 0, 0, 0, 0, 0, 37953, 0, 0, 0, 'Majordomo Staghelm (1)', 'Archdruid of the Flame', '', 0, 88, 88, 3, 14, 14, 0, 2, 1.71429, 1, 3, 70000, 80000, 0, 308, 1, 1250, 2000, 4, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 7, 108, 53856, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3000000, 3500000, '', 0, 3, 1330, 1, 1, 0, 0, 0, 0, 0, 0, 0, 169, 1, 0, 646922239, 1, '', 15595);
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (52593, 53859, 53860, 53861, 0, 0, 38747, 0, 0, 0, 'Spirit of the Flame', '', '', 0, 88, 88, 3, 14, 14, 0, 2, 1.14286, 1, 2, 45000, 55000, 0, 308, 1, 2000, 2000, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 2.46, 1, 1, 0, 0, 0, 0, 0, 0, 0, 191, 1, 0, 646922239, 0, 'spirit_of_flame_npc', 15595);
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (53859, 0, 0, 0, 0, 0, 38747, 0, 0, 0, 'Spirit of the Flame (1)', '', '', 0, 88, 88, 3, 14, 14, 0, 2, 1.14286, 1, 2, 55000, 65000, 0, 308, 1, 2000, 2000, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 8.61, 1, 1, 0, 0, 0, 0, 0, 0, 0, 191, 1, 0, 646922239, 0, '', 15595);


    DELETE FROM `spell_script_names` WHERE  spell_id=98620 OR  spell_id=100215 OR  spell_id=100216 OR  spell_id=100217;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98620, 'spell_gen_searing_seed'),
    (100215, 'spell_gen_searing_seed'),
    (100216, 'spell_gen_searing_seed'),
    (100217, 'spell_gen_searing_seed');

    DELETE FROM `spell_script_names` WHERE  spell_id=98450;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98450, 'spell_searing_seed_explosion');

    DELETE FROM `spell_script_names` WHERE  spell_id=99705 OR  spell_id=100101;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (99705, 'spell_gen_kneel_to_the_flame'),
    (100101, 'spell_gen_kneel_to_the_flame');


    DELETE FROM `spell_script_names` WHERE  spell_id=98474 OR  spell_id=100212 OR  spell_id=100213 OR  spell_id=100214;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98474, 'spell_gen_flame_scythe'),
    (100212, 'spell_gen_flame_scythe'),
    (100213, 'spell_gen_flame_scythe'),
    (100214, 'spell_gen_flame_scythe');

    DELETE FROM `spell_script_names` WHERE  spell_id=98535 OR  spell_id=100206 OR  spell_id=100207 OR  spell_id=100208;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98535, 'spell_gen_leaping_flames'),
    (100206, 'spell_gen_leaping_flames'),
    (100207, 'spell_gen_leaping_flames'),
    (100208, 'spell_gen_leaping_flames');

    UPDATE `creature_template` SET `ScriptName`='druid_of_the_flame' WHERE  `entry`=53619 LIMIT 1;

    UPDATE `creature_template` SET `mechanic_immune_mask`=646920191 WHERE  `entry`=53803 LIMIT 1;
    UPDATE `creature_template` SET `rank`=3 WHERE  `entry`=53859 LIMIT 1;
    UPDATE `creature_template` SET `rank`=3 WHERE  `entry`=52593 LIMIT 1;

    UPDATE `creature_template` SET `rank`=3 WHERE  `entry`=53619 LIMIT 1;
    UPDATE `creature_template` SET `rank`=3 WHERE  `entry`=53803 LIMIT 1;
    select * from creature_template where entry in (53619,53803);

    INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `quest_start`, `cast_flags`, `user_type`) VALUES (53696, 45576, 0, 1, 0);
*/
