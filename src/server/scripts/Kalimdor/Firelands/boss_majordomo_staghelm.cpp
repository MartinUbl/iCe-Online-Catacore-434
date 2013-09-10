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
};


struct Yells
{
    uint32 sound;
    const char * text;
};

static const Yells RandomKill[4]= // TODO -> Find sound id's
{
    {24477, "Burn."},
    {24479, "Soon, ALL of Azeroth will burn!"},
    {24480, "So much power!"},
    {24481, "You stood in the fire!"},
};

uint32 energyField[10] = {18,12,10,8,7,6,5,4,3,2}; // Very wierd calculation of time, so i do it manually for sure

#define MINUTE 60000
#define NEVER  (4294967295) // used as "delayed" timer ( max uint32 value)

#define MIDDLE_X  430.0f
#define MIDDLE_Y -63.0f
#define MIDDLE_Z  79.0f

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
        }

        InstanceScript* instance;
        uint8 PHASE;
        uint32 morphs;
        uint32 Morph_timer;
        uint32 Energy_timer;
        uint32 Human_timer;
        uint32 Berserk_timer;
        uint32 Phase_check_timer;
        uint32 energyCounter;
        bool FromCatToScorpion;

        SummonList Summons;

        void Reset()
        {
            energyCounter = 0;
            TransformToDruid();
            morphs = 0;
            PHASE = PHASE_DRUID;
            Phase_check_timer = 2000;
            Morph_timer = 2000;
            Energy_timer = 1000;
            Berserk_timer = 10 * MINUTE;
            Human_timer = NEVER;
        }

        void KilledUnit(Unit * /*victim*/)
        {
            uint8 rand_ = urand(0,3);
            PlayAndYell(RandomKill[rand_].sound,RandomKill[rand_].text);
        }

        void JustDied(Unit * /*victim*/)
        {
            Summons.DespawnAll();
            me->MonsterYell("My studies... had only just begun...", LANG_UNIVERSAL, 0);
            me->PlayDistanceSound(24464);
        }

        void EnterCombat(Unit* who)
        {
            morphs++;
            me->MonsterYell("Very well. Witness the raw power of my new lord!", LANG_UNIVERSAL, 0);
            me->PlayDistanceSound(24464);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,10.0f);
        }

        void EnterEvadeMode()
        {
            Summons.DespawnAll();
            TransformToDruid();
            ScriptedAI::EnterEvadeMode();
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

                if(unit && unit->isAlive() && unit->ToPlayer() && IsRangedClass(unit->ToPlayer()) )
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
            uint32 time_gap = (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL) ? (55/23) : (55/9); // (55 seconds / number of players ) --> tanks are excluded
            time_gap *= 1000; // Need miliseconds

            std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
            for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
            {
                Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());

                if(unit && unit->ToPlayer()) // Seering seeds cannot be applied at current tank ( handled in spellscript)
                {
                    Aura * seed_aura = unit->GetAura(SPELL_SEARING_SEEDS);

                    if(seed_aura)
                    {
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

        bool RaidIsClusteredTogether(void) // If >= 7/18 (10 man / 25 man) people are clustered together, switch to Scorpion form 
        {
            uint32 clustered_people = 0;

            std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
            for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
            {
                Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());
                if ( unit && (unit->GetTypeId() == TYPEID_PLAYER) && unit->isAlive())
                    if( me->getVictim()->GetExactDist2d(unit->GetPositionX(),unit->GetPositionY()) <= 15.0f) // 15 yards from tank
                        clustered_people++;
            }

            if( getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL )
                return (clustered_people >= 18) ? true : false;

            return (clustered_people >= 7) ? true : false;

        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim() || !me->getVictim())
                return;

            if(Phase_check_timer <= diff)
            {
                if(morphs == 3)
                {
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

                if(RaidIsClusteredTogether())
                {
                    if(!me->HasAura(SPELL_SCORPION_FORM))
                        TransformToScorpion();
                }
                else
                {
                    if(!me->HasAura(SPELL_CAT_FORM))
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
                    uint8 max = (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL) ? 5 : 2;

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

                if(me->getPowerType() == POWER_ENERGY)
                {
                        adrenalineStacks = (adrenalineStacks > 9) ? 9 : adrenalineStacks; // Out of bounds protection
                        me->SetPower(POWER_ENERGY,me->GetPower(POWER_ENERGY) + (100 / energyField[adrenalineStacks]));
                }
                Energy_timer = 1000;
            }
            else Energy_timer -= diff;


            /*if(Morph_timer <= diff)
            {
                if(morphs == 3)
                {
                    FromCatToScorpion = me->HasAura(SPELL_CAT_FORM);
                    TransformToDruid();
                    me->CastSpell(me,SPELL_FIERY_CYCLONE,true);

                    if(FromCatToScorpion) // if we are transforming from cat to scorpion
                    {
                        me->CastSpell(me,SPELL_SEARING_SEEDS,false); // cast seering seeds
                        me->MonsterYell("Blaze of Glory!", LANG_UNIVERSAL, 0);
                    }
                    else                  // if we are transforming from scorpion to cat
                    {
                        me->CastSpell(me,SPELL_BURNING_ORBS,false); // cast Burning orbs
                        me->MonsterYell("Nothing but ash!", LANG_UNIVERSAL, 0);
                    }

                    Human_timer = 4050;
                    Morph_timer = 4000;
                    morphs = 0;
                    return;
                }

                if(me->HasAura(SPELL_SCORPION_FORM))
                    TransformToCat(); 
                else
                    TransformToScorpion();

                Morph_timer = MINUTE/2;
            }
            else Morph_timer -= diff;*/

            if(Berserk_timer <= diff) // Berser after 10 minutes
            {
                me->CastSpell(me,SPELL_BERSERK,true);
                Berserk_timer = NEVER;
            }
            else Berserk_timer -= diff;


            if( PHASE == PHASE_SCORPION && me->GetPower(POWER_ENERGY) == 100)
            {
                me->CastSpell(me,SPELL_FLAME_SCYTE,false);
            }

            if( PHASE == PHASE_CAT && me->GetPower(POWER_ENERGY) == 100)
            {
                me->CastSpell(me,SPELL_SUMMON_SPIRIT,false); // Summon 1 cat copy of Majordomo ( cat)
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
        }

        void Reset()
        {
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();

            if ( Unit* player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true) )
                me->GetMotionMaster()->MoveChase(player);
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


class IsTankingMajordomo // Seering seeds cannot be applied at current tank
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object && object->ToPlayer())
                if (Creature* majordomo = GetClosestCreatureWithEntry(object, MAJORDOMO_STAGHELM, 500.0f,true))
                    if(majordomo->getVictim() == object )
                        return true;
            return false;
        }
};

class spell_gen_searing_seed : public SpellScriptLoader
{
    public:
        spell_gen_searing_seed() : SpellScriptLoader("spell_gen_searing_seed") { }

        class spell_gen_searing_seed_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_searing_seed_SpellScript);

            void RemoveInvalidTargets(std::list<Unit*>& unitList)
            {
                unitList.remove_if(IsTankingMajordomo());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_gen_searing_seed_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_searing_seed_SpellScript();
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
                    if ((*itr)->isGuardian() || (*itr)->isPet())
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

    // SPELLS
    new spell_gen_searing_seed(); //        98450
    new spell_searing_seed_explosion(); //  98620,100215,100216,100217
    new spell_gen_flame_scythe(); //        98474,100212,100213,100214
    new spell_gen_leaping_flames(); //      98535,100206,100207,100208
}

/*
    // TODO -> ADD CREATURE QUERIES !!!!

    DELETE FROM `spell_script_names` WHERE  spell_id=98450;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98450, 'spell_gen_searing_seed');

    DELETE FROM `spell_script_names` WHERE  spell_id=98450;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
    VALUES (98450, 'spell_searing_seed_explosion');

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
*/