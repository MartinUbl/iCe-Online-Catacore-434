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
    BERSERK                     = 26662,
    BLAZE_OF_GLORY              = 99252,
    INCENDIARY_SOUL             = 99369,

    DECIMATION_BLADE            = 99352,
    DECIMATION_BLADE_TRIGGERED  = 99353,
    INFERNO_BLADE               = 99350,
    INFERNO_BLADE_TRIGGERED     = 99351,

    VITAL_SPARK                 = 99262,
    VITAL_FLAME                 = 99263,

    SUMMON_SHARD_DUMMY          = 99259,
    SUMMON_SHARD                = 99260,

    // BRIDGE !!!
    BRIDGE_FORMING_ANIM         = 101035, // are you serious blizz, really spell ? (damn)

    // HEROIC SPELLS
    COUNTDOWN_TIMER             = 99516,
    COUNTDOWN_SELECTER          = 99517,
    COUNTDOWN_LINK              = 99519,
    COUNTDOWN_DAMAGE            = 99518

};

struct Yells
{
    uint32 sound;
    const char * text;
};

static const Yells onAggro = {24441,"You are forbidden from my master's domain, mortals."};

static const Yells onShard = {24446,"Fool mortals. Hurl yourselves into your own demise!"};

static const Yells onInfernoBlade = {24459,"Burn beneath my molten fury!"};

static const Yells onDecimationBlade = {24447,"By the Firelord's command, you, too, shall perish!"};

static const Yells onBerserk = {24450,"Your flesh is forfeit to the fires of this realm."};

static const Yells onDeath = {24444,"Mortal filth... the master's keep is forbidden..."};


static const Yells onKill[3]=
{
    {24449, "You have been judged."},
    {24451, "Behold your weakness."},
    {24452, "None shall pass!"},
};

enum actions
{
    DO_EQUIP_INFERNO_BLADE              = 0,
    DO_EQUIP_DECIMATION_BLADE           = 1,
    DO_EQUIP_NORMAL_BLADE               = 2,
};

# define MINUTE (60000)
# define NEVER  (0xffffffff) // used as "delayed" timer ( max uint32 value )


enum ItemEntries // Thanks Gregory for help with sniffing :)
{
    NORMAL_BLADE_ENTRY        = 71055,
    INFERNO_BLADE_ENTRY       = 94155,
    DECIMATION_BLADE_ENTRY    = 94157,
};

class boss_baleroc : public CreatureScript
{
public:
    boss_baleroc() : CreatureScript("boss_baleroc") { }
  
    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_balerocAI(pCreature);
    }

    struct boss_balerocAI : public ScriptedAI
    {
        boss_balerocAI(Creature *c) : ScriptedAI(c),Summons(c)
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            instance = me->GetInstanceScript();

            SetEquipmentSlots(false, NORMAL_BLADE_ENTRY, NORMAL_BLADE_ENTRY, EQUIP_NO_CHANGE); // Set blades to both hands
            GameObject * bridgeDoor= me->SummonGameObject(208906,126.92f,-63.55f,55.27f,2.5823f,0,0,0,0,0); // Fire wall

            if (bridgeDoor)
                instance->SetData64(DATA_BRIDGE_DOOR,bridgeDoor->GetGUID());
        }

        uint32 blazeOfGloryTimer;
        uint32 bladeTimer;
        uint32 berserkTimer;
        uint32 castShardTimer;
        uint32 summonShardTimer;
        uint32 countDownTimer;
        bool meleePhase;

        InstanceScript * instance;
        SummonList Summons;

        void Reset()
        {
            if(instance)
            {
                instance->SetData(DATA_PAIN_ACHIEV, 1);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                instance->SetData(TYPE_BALEROC, NOT_STARTED);
            }

            castShardTimer          = 4000;
            summonShardTimer        = NEVER;
            blazeOfGloryTimer       = 8000;
            countDownTimer          = 27000;
            bladeTimer              = 28000;
            berserkTimer            = 6 * MINUTE;
            meleePhase = true;
        }

        void JustSummoned(Creature* summon)
        {
            Summons.push_back(summon->GetGUID());
        }

       void KilledUnit(Unit * /*victim*/)
       {
           uint8 _rand = urand(0,2);
           PlayAndYell(onKill[_rand].sound,onKill[_rand].text);
       }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                if (instance->GetData(DATA_PAIN_ACHIEV) == 1)
                    instance->DoCompleteAchievement(5830); // Share the Pain

                instance->SetData(DATA_BRIDGE_SPAWN, 37000);
            }


            PlayAndYell(onDeath.sound,onDeath.text);

            if (GameObject * door1 = me->FindNearestGameObject(208906,200.0f))
                door1->Delete();

            Summons.DespawnAll();
            RemoveBlazeOfGloryFromPlayers();

            if (instance)
            {
                instance->SetData(TYPE_BALEROC,DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE,me);
            }

            //me->SummonGameObject(5010734,247.0f,-64.0f,62.0f,3.15f,0,0,0,0,0); // Bridge
            me->SummonCreature(54101,100.0f,-33.0f,61.0f,4.34f,TEMPSUMMON_CORPSE_DESPAWN, 0); // Staghelm intro
        }

        void EnterCombat(Unit * /*who*/)
        {
            SetEquipmentSlots(false, NORMAL_BLADE_ENTRY, NORMAL_BLADE_ENTRY, EQUIP_NO_CHANGE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,20.0f);
            me->SetInCombatWithZone();
            RemoveBlazeOfGloryFromPlayers(); // For sure

            if(instance)
            {
                 instance->SetData(TYPE_BALEROC,IN_PROGRESS);
                 instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            PlayAndYell(onAggro.sound,onAggro.text);
        }

        void EnterEvadeMode()
        {
            me->SetFloatValue(UNIT_FIELD_COMBATREACH,5.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS,20.0f);
            Summons.DespawnAll();
            RemoveBlazeOfGloryFromPlayers();
            SetEquipmentSlots(false, NORMAL_BLADE_ENTRY, NORMAL_BLADE_ENTRY, EQUIP_NO_CHANGE);

            if(instance)
            {
                instance->SetData(TYPE_BALEROC,NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            ScriptedAI::EnterEvadeMode();
        }

        void DoAction(const int32 action) // Called in aurascript
        {
            switch(action)
            {
                case DO_EQUIP_INFERNO_BLADE:
                    SetEquipmentSlots(false, INFERNO_BLADE_ENTRY,EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                    me->SetSheath(SHEATH_STATE_MELEE);
                    meleePhase = false;
                break;

                case DO_EQUIP_DECIMATION_BLADE:
                    SetEquipmentSlots(false, DECIMATION_BLADE_ENTRY,EQUIP_UNEQUIP, EQUIP_NO_CHANGE);
                    me->SetSheath(SHEATH_STATE_MELEE);
                    me->setAttackTimer(BASE_ATTACK, 4500);
                    me->setAttackTimer(OFF_ATTACK, 4500);
                    meleePhase = false;
                break;

                case DO_EQUIP_NORMAL_BLADE:
                    SetEquipmentSlots(false, NORMAL_BLADE_ENTRY, NORMAL_BLADE_ENTRY, EQUIP_NO_CHANGE);
                    me->SetSheath(SHEATH_STATE_MELEE);
                    me->setAttackTimer(BASE_ATTACK, 2000);
                    me->setAttackTimer(OFF_ATTACK, 2000);
                    meleePhase = true;
                break;

                default:
                    break;
            }
        }

        void PlayAndYell(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me,soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void RemoveBlazeOfGloryFromPlayers(void)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();

            if (plList.isEmpty())
                return;

            for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if ( Player * p = itr->getSource())
                    p->RemoveAurasDueToSpell(BLAZE_OF_GLORY);
            }
        }

        void SpawnShardOfTorment(bool _10man)
        {
            if(_10man)
            {
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 2, 100.0f, true) ) // Not on tanks if possible
                    me->CastSpell(player,SUMMON_SHARD,true);
                else if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true) )
                        me->CastSpell(player,SUMMON_SHARD,true);
                return;
            }

            //25 man -> spawn 2 shards

            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();

            if (plList.isEmpty())
                return;

            std::list<Player*> shard_list;

            for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                    if (!p->HasTankSpec() && !p->HasAura(5487)) // Bear form
                        shard_list.push_back(p);
                }
            }

            if(!shard_list.empty() && shard_list.size() >= 2 )
            {
                std::list<Player*>::iterator j = shard_list.begin();
                advance(j, rand()%shard_list.size());

                if (*j && (*j)->IsInWorld() == true )
                    me->CastSpell((*j),SUMMON_SHARD,true);

                shard_list.erase(j);
                j = shard_list.begin();

                advance(j, rand()%shard_list.size());
                if (*j && (*j)->IsInWorld() == true )
                    me->CastSpell((*j),SUMMON_SHARD,true);
            }
            else // If only tanks are alive, select random
            {
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true) )
                    me->CastSpell(player,SUMMON_SHARD,true);
                if (Unit * player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true) )
                        me->CastSpell(player,SUMMON_SHARD,true);
            }
        }

        void SelectCountDownVictims(void)
        {
            Map * map = me->GetMap();

            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            std::list<Player*> countdownTargets;
            countdownTargets.clear();

            for(Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if(Player* pl = itr->getSource())
                {
                    if ( pl && pl->IsInWorld() && pl->isAlive() && !pl->isGameMaster() && pl->GetDistance(me) < 250.0f)
                    {
                        if (!pl->HasTankSpec() && !pl->HasAura(5487)) // Bear form
                        {
                            countdownTargets.push_back(pl);
                        }
                    }
                }
            }

            if(countdownTargets.size() >= 2 )
            {
                Player * pl1 = NULL, *pl2 = NULL;

                std::list<Player*>::iterator j = countdownTargets.begin();
                advance(j, rand()%countdownTargets.size());
                if (*j)
                    pl1 = *j;

                countdownTargets.erase(j);
                j = countdownTargets.begin();

                advance(j, rand()%countdownTargets.size());
                if (*j)
                    pl2 = *j;

                if (pl1 && pl2 && pl1->IsInWorld() && pl2->IsInWorld())
                {
                    pl1->CastSpell(pl1,COUNTDOWN_TIMER,false);
                    pl2->CastSpell(pl2,COUNTDOWN_TIMER,false);

                    pl1->CastSpell(pl2,COUNTDOWN_LINK,false); // Visual link between players
                }

            }

        }

       void UpdateAI(const uint32 diff)
       {
            if (!UpdateVictim())
                return;

            if (!me->IsWithinLOS(me->getVictim()->GetPositionX(),me->getVictim()->GetPositionY(),me->getVictim()->GetPositionZ()))
                me->Kill(me->getVictim(),true);

            if (countDownTimer <= diff)
            {
                if (IsHeroic())
                    SelectCountDownVictims();
                countDownTimer = 48000;
            }
            else countDownTimer -= diff;

            if (blazeOfGloryTimer <= diff)
            {
                if(me->getVictim()->ToPlayer())
                    me->CastSpell(me->getVictim(),BLAZE_OF_GLORY,true);
                me->CastSpell(me, INCENDIARY_SOUL, true);
                blazeOfGloryTimer = urand(8000,13000);
            }
            else blazeOfGloryTimer -= diff;

            if (berserkTimer <= diff)
            {
                PlayAndYell(onBerserk.sound,onBerserk.text);
                me->CastSpell(me,BERSERK,true);
                berserkTimer = NEVER;
            }
            else berserkTimer -= diff;

/*****************************************************************************************/

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            if (castShardTimer <= diff)
            {
                PlayAndYell(onShard.sound,onShard.text);
                me->CastSpell(me,SUMMON_SHARD_DUMMY,false); // 1.5 s cast time ( just dummy )
                castShardTimer = urand(31000,33000);
                summonShardTimer = 1500;
                return;
            }
            else castShardTimer -= diff;

            if (summonShardTimer <= diff)
            {
                if(instance)
                {
                    if(getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC)
                        SpawnShardOfTorment(false);
                    else
                        SpawnShardOfTorment(true);
                }

                summonShardTimer = NEVER;
                return;
            }
            else summonShardTimer -= diff;

            if (bladeTimer <= diff)
            {
                if (urand(0,1))
                {
                    PlayAndYell(onInfernoBlade.sound,onInfernoBlade.text);
                    me->CastSpell(me,INFERNO_BLADE,false);
                }
                else
                {
                    PlayAndYell(onDecimationBlade.sound,onDecimationBlade.text);
                    me->CastSpell(me,DECIMATION_BLADE,false);
                }

                bladeTimer = 30000;
                return;
            }
            else bladeTimer -= diff;

/**********************************AUTO ATTACK STUFF***********************************************/

            if(meleePhase) // Melee attack phase
                DoMeleeAttackIfReady();
            else // Spell attack phase
            {
                if (me->hasUnitState(UNIT_STAT_CASTING))
                    return;

                if (me->isAttackReady())
                {
                    if(me->HasAura(99352) || me->HasAura(99405) ) // Decimation Blade
                    {
                        if (me->IsWithinCombatRange(me->getVictim(), GetSpellMaxRange(99353, false)))
                        {
                            me->setAttackTimer(BASE_ATTACK, 4500);
                            me->setAttackTimer(OFF_ATTACK, 4500);

                            bool avoided = false; // Decimation blade can be only dodged or parried

                            if (roll_chance_f(me->getVictim()->GetUnitDodgeChance()))
                                avoided = true;

                            if (roll_chance_f(me->getVictim()->GetUnitParryChance()))
                                avoided = true;

                            int32 bp0 = me->getVictim()->CountPctFromMaxHealth(90);

                            if (me->getVictim()->CountPctFromMaxHealth(90) < 250000 )
                                bp0 = 250000;

                            if(avoided)
                                bp0 = 0;

                            me->CastCustomSpell(me->getVictim(),99353,&bp0,0,0,true); // Decimation Strike

                            if (avoided)
                                me->getVictim()->RemoveAurasDueToSpell(99353);

                            //me->resetAttackTimer();
                        }
                        return;
                    }

                    if (me->HasAura(99350)) // Inferno Blade
                    {
                        if (me->IsWithinCombatRange(me->getVictim(), GetSpellMaxRange(99351, false)))
                        {
                            me->setAttackTimer(BASE_ATTACK, 1800);
                            me->setAttackTimer(OFF_ATTACK, 1800);

                            bool avoided = false; // Inferno blade can be dodged,parried or blocked

                            if (roll_chance_f(me->getVictim()->GetUnitDodgeChance()))
                                avoided = true;

                            if (roll_chance_f(me->getVictim()->GetUnitParryChance()))
                                avoided = true;

                            if (roll_chance_f(me->getVictim()->GetUnitBlockChance()))
                                avoided = true;

                            if(avoided)
                                me->CastCustomSpell(me->getVictim(),99351,0,0,0,true);
                            else
                                me->CastSpell(me->getVictim(),99351,true); // Inferno strike

                            //me->resetAttackTimer();
                        }
                    }
                }
            }

       }
    };
};


enum shardSpells
{
    TORMENT_BEAM                    = 99255,
    SHARD_VISUAL                    = 99254,
    WAVE_OF_TORMENT                 = 99261,
    TORMENTED_DEBUFF                = 99257,
    TORMENT_VISUAL_BEAM             = 99258
};

enum shardActions
{
    SOMEONE_IN_RANGE    = 0,
    NO_ONE_IN_RANGE     = 1,
};

// SHARD OF TORMENT AI
class npc_shard_of_torment: public CreatureScript
{
public:
    npc_shard_of_torment(): CreatureScript("npc_shard_of_torment") {}

    struct npc_shard_of_tormentAI: public ScriptedAI
    {
        npc_shard_of_tormentAI(Creature* c): ScriptedAI(c)
        {
            Reset();
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
            pInstance = me->GetInstanceScript();
        }

        bool canWave;
        uint32 waveTimer;
        uint32 beamTimer;
        uint32 checkTimer;
        InstanceScript * pInstance;


        void Reset()
        {
            canWave = false;
            waveTimer = 6000;
            checkTimer = 5000;
            beamTimer = 4000;
            me->CastSpell(me,TORMENT_VISUAL_BEAM,true);
            me->SetInCombatWithZone();
        }

        void DoAction(const int32 action)
        {
            if (action == NO_ONE_IN_RANGE)
            {
                canWave = true;
            }
            else if ( action == SOMEONE_IN_RANGE)
            {
                waveTimer = 1000;
                canWave = false;
            }
        }

        Player* SelectClosestPlayer()
        {
            if (!pInstance)
                return NULL;

            Map::PlayerList const& plList = pInstance->instance->GetPlayers();

            if (plList.isEmpty())
                return NULL;

            float min_range = FLT_MAX;
            Player* minrangeplayer = NULL;
            for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (!itr->getSource())
                    continue;

               if ( !itr->getSource()->isAlive())
                    continue;

                if (me->GetDistance2d(itr->getSource()) < min_range)
                {
                    min_range = me->GetExactDist2d(itr->getSource());
                    minrangeplayer = itr->getSource();
                }
            }

            return minrangeplayer;
        }

        bool SomeoneInRange(void)
        {
            if (!pInstance)
                return NULL;

            Map::PlayerList const& plList = pInstance->instance->GetPlayers();

            if (plList.isEmpty())
                return NULL;

            for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (!itr->getSource())
                    continue;

                if ( !itr->getSource()->isAlive())
                    continue;

                if (me->GetDistance2d(itr->getSource()) <= 14.0f) // 15 yards ( - 1 yard )
                    return true;
            }

            return false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkTimer <= diff) // Check if someone is in 15 yards range from Crystal
            {
                if (SomeoneInRange())
                    DoAction(SOMEONE_IN_RANGE);
                else
                    DoAction(NO_ONE_IN_RANGE);

                checkTimer = 1500;
            }
            else checkTimer -= diff;

            if (waveTimer <= diff)
            {
                if(canWave)
                    me->CastSpell(me,WAVE_OF_TORMENT,true);

                waveTimer = 1000;
            }
            else waveTimer -= diff;

            if (beamTimer <= diff)
            {
                if (me->HasAura(TORMENT_VISUAL_BEAM))
                {
                    me->RemoveAurasDueToSpell(TORMENT_VISUAL_BEAM);
                    me->CastSpell(me,SHARD_VISUAL,true);
                }

                Player * shardTaregt = SelectClosestPlayer();

                if (shardTaregt && shardTaregt->IsInWorld() &&  !shardTaregt->HasAura(TORMENT_BEAM))
                    me->CastSpell(shardTaregt,TORMENT_BEAM,false);

                beamTimer = 1000;
            }
            else beamTimer -= diff;

        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new npc_shard_of_tormentAI(c);
    }
};

class spell_gen_tormented : public SpellScriptLoader
{
public:
    spell_gen_tormented() : SpellScriptLoader("spell_gen_tormented") { }

    class spell_gen_tormented_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_tormented_AuraScript);

        void OnExpire(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            Unit * target = aurEff->GetBase()->GetUnitOwner();

            if(!target)
                return;

            target->CastSpell(target,TORMENTED_DEBUFF,true); // If torment fades from player, cast tormented debuff on him
            Aura * aTorm = target->GetAura(TORMENTED_DEBUFF);
            if (!aTorm)
                aTorm = target->GetAura(99402);
            if (!aTorm)
                aTorm = target->GetAura(99403);
            if (!aTorm)
                aTorm = target->GetAura(99404);

            if (aTorm)
            {
                if (target->GetInstanceScript() != NULL)
                {
                    if (aTorm->GetId() == TORMENTED_DEBUFF || aTorm->GetId() == 99403) // 10 man
                        aTorm->SetDuration(40000);
                    else                                  // 25 man
                        aTorm->SetDuration(60000);
                }
            }

        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_tormented_AuraScript::OnExpire, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_tormented_AuraScript();
    }
};

class spell_gen_baleroc_blades : public SpellScriptLoader
{
public:
    spell_gen_baleroc_blades() : SpellScriptLoader("spell_gen_baleroc_blades") { }

    class spell_gen_baleroc_blades_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_baleroc_blades_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            Unit * caster= aurEff->GetCaster();

            if(!caster || !caster->ToCreature())
                return;

            if (GetSpellProto()->Id == 99350)
                caster->ToCreature()->AI()->DoAction(DO_EQUIP_INFERNO_BLADE);
            else
                caster->ToCreature()->AI()->DoAction(DO_EQUIP_DECIMATION_BLADE);
        }

        void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            Unit * caster= aurEff->GetCaster();

            if(!caster || !caster->ToCreature())
                return;

            caster->ToCreature()->AI()->DoAction(DO_EQUIP_NORMAL_BLADE);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_gen_baleroc_blades_AuraScript::OnApply, EFFECT_FIRST_FOUND, SPELL_AURA_361, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_baleroc_blades_AuraScript::OnRemove, EFFECT_FIRST_FOUND, SPELL_AURA_361, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_baleroc_blades_AuraScript();
    }
};

class spell_gen_torment : public SpellScriptLoader
{
    public:
        spell_gen_torment() : SpellScriptLoader("spell_gen_torment") { }

        class spell_gen_torment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_torment_SpellScript);

            bool stored;

            bool Load()
            {
                stored = false;
                return true;
            }

            uint32 GetSpellId(Unit * caster)
            {
                InstanceScript * pInstance= caster->GetInstanceScript();

                if (pInstance)
                {
                    switch (pInstance->instance->GetDifficulty())
                    {
                        case RAID_DIFFICULTY_10MAN_NORMAL:
                            return 99256;
                        case RAID_DIFFICULTY_25MAN_NORMAL:
                            return 100230;
                        case RAID_DIFFICULTY_10MAN_HEROIC:
                            return 100231;
                        case RAID_DIFFICULTY_25MAN_HEROIC:
                            return 100232;

                        default:
                            return 0;
                    }
                }
                return 0;
            }

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                Unit * caster  = GetCaster();
                Unit * target  = GetHitUnit();

                if (!caster || !target)
                    return;

                uint32 spellId = GetSpellId(caster);

                if(spellId)
                {
                    uint32 stacks = target->GetAuraCount(spellId);

                    if (stacks >= 4)
                        if (InstanceScript * pInstance = caster->GetInstanceScript())
                            pInstance->SetData(DATA_PAIN_ACHIEV, 0); // Achiev failed

                    if(stacks)
                        SetHitDamage( (500 + GetHitDamage()) * stacks); // pre nerfed value
                }
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_gen_torment_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_torment_SpellScript();
        }
};

class spell_gen_vital_flame : public SpellScriptLoader
{
public:
    spell_gen_vital_flame() : SpellScriptLoader("spell_gen_vital_flame") { }

    class spell_gen_vital_flame_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_vital_flame_AuraScript);

        void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if(GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                return;

            Unit * caster= aurEff->GetCaster();

            if(!caster)
                return;

            if(aurEff->GetAmount() > 0)
            {
                uint32 returnedSparks = uint32(aurEff->GetAmount() / 5);

                for(uint32 i = 0; i < returnedSparks; i++)
                    caster->CastSpell(caster,99262,true); // Vital Spark
            }
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_gen_vital_flame_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_359, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_gen_vital_flame_AuraScript();
    }
};

// HEROIC SPELLS
class spell_gen_countdown_Selector : public SpellScriptLoader
{
    public:
        spell_gen_countdown_Selector() : SpellScriptLoader("spell_gen_countdown_Selector") { }

        class spell_gen_countdown_Selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_countdown_Selector_SpellScript);

            void HandleScript()
            {
                Unit * hitUnit = GetHitUnit();
                Unit * caster = GetCaster();

                if (!hitUnit || !caster)
                    return;

                Aura *a = hitUnit->GetAura(99516);
                if (a)
                    a->Remove(AURA_REMOVE_BY_DEFAULT);
                a = caster->GetAura(99516);
                if (a)
                    a->Remove(AURA_REMOVE_BY_DEFAULT);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_gen_countdown_Selector_SpellScript::HandleScript);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_countdown_Selector_SpellScript();
        }
};

class spell_gen_tormented_Selector : public SpellScriptLoader
{
    public:
        spell_gen_tormented_Selector() : SpellScriptLoader("spell_gen_tormented_Selector") { }

        class spell_gen_tormented_Selector_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_tormented_Selector_SpellScript);

            void HandleScript()
            {
                Unit * hitUnit = GetHitUnit();

                if (!hitUnit)
                    return;

                hitUnit->AddAura(TORMENTED_DEBUFF,hitUnit);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_gen_tormented_Selector_SpellScript::HandleScript);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_tormented_Selector_SpellScript();
        }
};

class npc_majordomo_intro_npc : public CreatureScript
{
public:
   npc_majordomo_intro_npc() : CreatureScript("npc_majordomo_intro_npc") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_majordomo_intro_npcAI (creature);
    }

    struct npc_majordomo_intro_npcAI : public ScriptedAI
    {
        npc_majordomo_intro_npcAI(Creature* creature) : ScriptedAI(creature) 
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            instance = me->GetInstanceScript();
            //me->SetVisible(false);
        }

        uint32 talkTimer;
        uint32 talks;
        InstanceScript * instance;

        void Reset()
        {
            talkTimer = 5000;
            me->SetReactState(REACT_PASSIVE);
            talks = 0;
        }

        void PlayCinematicToPlayers(void)
        {
            if (!instance)
                return;

            Map::PlayerList const& plList = instance->instance->GetPlayers();

            if (plList.isEmpty())
                return;

            for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if ( Player * p = itr->getSource())
                    p->SendCinematicStart (197);
            }
        }

        void UpdateAI (const uint32 diff)
        {
            if(talkTimer <= diff)
            {
                switch (talks)
                {
                    case 0 :
                        me->MonsterYell("Well well... I admire your tenacity. Baleroc stood guard over this keep for a thousand mortal lifetimes.",LANG_UNIVERSAL,0);
                        me->PlayDistanceSound(24473);
                        talkTimer = 12000;
                        break;
                    case 1:
                        me->MonsterYell("But NONE may enter the Firelord's abode!",LANG_UNIVERSAL,0);
                        me->PlayDistanceSound(24474);
                        talkTimer = 6500;
                        break;
                    case 2:
                        PlayCinematicToPlayers();
                        if (Creature * dummy = me->SummonCreature(540100,247.0f,-64.0f,62.0f,0.01f,TEMPSUMMON_TIMED_DESPAWN,20000))
                            dummy->CastSpell(dummy,BRIDGE_FORMING_ANIM,true);
                        me->MonsterYell("Beg for mercy now, and I may yet allow you to live. Well, \"heroes\", what is your answer?",LANG_UNIVERSAL,0);
                        me->PlayDistanceSound(24475);
                        talkTimer = 10000;
                        break;
                    case 3:
                        me->SetVisible(false);
                        talkTimer = 10000;
                        break;
                }

                talks++;

                if (talks == 4)
                    me->ForcedDespawn(1000);

            }
            else talkTimer -= diff;
        }
    };
};

void AddSC_boss_baeloroc()
{
    new boss_baleroc();
    new npc_shard_of_torment();
    new npc_majordomo_intro_npc();

    new spell_gen_tormented(); // 99255
    new spell_gen_baleroc_blades(); // 99352,99405,99350
    new spell_gen_torment(); // 99256,100230,100231,100232
    new spell_gen_vital_flame(); // 99263
    // Heroic
    new spell_gen_countdown_Selector(); // 99517
    new spell_gen_tormented_Selector(); // 99489
}

/* HEROIC SQLs

    delete from spell_script_names where spell_id = 99517;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (99517, 'spell_gen_countdown_Selector');

    delete from spell_script_names where spell_id = 99489;
    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (99489, 'spell_gen_tormented_Selector');

*/

/*
    UPDATE `creature_equip_template` SET `equipentry1`=0, `equipentry2`=0 WHERE  `entry`=53494 LIMIT 1;

    UPDATE `creature_template` SET `ScriptName`='boss_baleroc' WHERE  `entry`=53494 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='boss_baleroc' WHERE  `entry`=53587 LIMIT 1;

    UPDATE `creature_template` SET `minlevel`=88, `maxlevel`=88, `exp`=3, `faction_A`=14, `faction_H`=14, `ScriptName`='npc_shard_of_torment' WHERE  `entry`=53495 LIMIT 1;
    UPDATE `creature_template` SET `modelid1`=11686, `modelid2`=11686, `modelid3`=11686 WHERE  `entry`=53495 LIMIT 1;
    UPDATE `creature_template` SET `scale`=1 WHERE  `entry`=53495 LIMIT 1;

    UPDATE `creature_template` SET `mindmg`=80000, `maxdmg`=90000 WHERE  `entry`=53494 LIMIT 1;
    UPDATE `creature_template` SET `mindmg`=80000, `maxdmg`=90000 WHERE  `entry`=53587 LIMIT 1;
    UPDATE `creature_template` SET `Health_mod`=311.75 WHERE  `entry`=53494 LIMIT 1;

    delete from spell_script_names where spell_id in (99255,99352,99405,99350);

     INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (99255, 'spell_gen_tormented'),
      (99352, 'spell_gen_baleroc_blades'),
      (99405, 'spell_gen_baleroc_blades'),
      (99350, 'spell_gen_baleroc_blades');

    delete from spell_script_names where spell_id in (99256,100230,100231,100232);

     INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (99256, 'spell_gen_torment'),
      (100230, 'spell_gen_torment'),
      (100231, 'spell_gen_torment'),
      (100232, 'spell_gen_torment');

    delete from spell_script_names where spell_id = 99263;

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
     VALUES (99263, 'spell_gen_vital_flame');

     UPDATE `gameobject_template` SET `ScriptName`='go_firelands_bridge_opener' WHERE  `entry`=209277 LIMIT 1;

     UPDATE `creature_template` SET `ScriptName`='npc_majordomo_intro_npc' WHERE  `entry`=54101 LIMIT 1;
*/
