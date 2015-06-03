#include "ScriptPCH.h"
#include "blackwing_descent.h"

enum Spells
{
    // PHASE 1 ABBILITIES
    FAST_ASLEEP           = 82706,
    CAUSTIC_SLIME_MISSILE = 82913,
    MASSACRE              = 82848,
    FEUD                  = 88872,
    BREAK                 = 82881,
    DOUBLE_ATTACK         = 82882,
    DOUBLE_ATTACK_BUFF    = 88826, // Dummy aura ?
    SPELL_BERSERK         = 47008,

    // PHASE 2 ABBILITIES
    MORTALITY_HEALING_DEBUFF = 82890,
    MORTALITY_BOSS_BUFF      = 82934,

    // HEROIC 
    MOCKING_SHADOWS          = 91307,
    SHADOW_WHIP              = 91304,

    // BILE O TRON ABBILITIES
    SYSTEMS_FAILURE          = 88853,
    REROUTE_POWER            = 88861,
    FINKLE_MIXTURE           = 82705,
    FINKLE_MIXTURE_SPRAY     = 91106,
};

enum CREATURES
{
    BILE_O_TRON = 44418,
    FINKLE      = 44202,
    CHIMAERON   = 43296,
    NEFARIUS    = 49580, // Fake Nef ? :) Orginal is 48964
};

class boss_chimaeron : public CreatureScript
{
public:
    boss_chimaeron() : CreatureScript("boss_chimaeron") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_chimaeronAI (creature);
    }

    struct boss_chimaeronAI : public ScriptedAI
    {
        boss_chimaeronAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Derelax_timer;
        uint32 Massacre_timer;
        uint32 Feud_timer;
        uint32 Slime_timer;
        uint32 Break_timer;
        uint32 Berserk_timer;
        uint32 Delay_melee_timer;
        uint32 Whip_timer;

        bool can_feud;
        bool feuding;
        bool whiped;

        uint32 Mass_counter;
        uint32 Slime_counter;
        uint32 PHASE;
        uint32 achiev_counter;

        Creature *Nef;
        void Reset()
        {
            achiev_counter = 0;
            Nef = NULL;
            PHASE = 1;
            if(!me->HasAura(FAST_ASLEEP) && !me->isMoving())
                DoCast(me,FAST_ASLEEP);

            can_feud = feuding = whiped = false;
            Mass_counter = Slime_counter = 0;
            Break_timer = 4500;
            Slime_timer = 10000,
            Massacre_timer = 26000;
            Feud_timer = Delay_melee_timer = Derelax_timer = Whip_timer= 999999; // Not now :D
            Berserk_timer = 450 * IN_MILLISECONDS; // Berserk after 7:30
            me->SetReactState(REACT_DEFENSIVE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 15);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 40);

            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            me->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            me->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
            me->ApplySpellImmune(0, IMMUNITY_ID, 77606, true); // Dark Simulacrum 
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);

            if (instance)
                instance->SetData(DATA_CHIMAERON_GUID, NOT_STARTED);
        }

        void JustReachedHome()
        {
            DoCast(me,FAST_ASLEEP);
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
             if(attacker->ToPlayer() && attacker->ToPlayer()->IsGameMaster() )
                    return;

             if (damage > 500000 ) // Anti IK
                damage=0; // What if :)
        }

        void KilledUnit(Unit * victim)
        {
            if(victim && victim->ToPlayer())
                achiev_counter++;
        }

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveAllAuras(); // Due to spell's  with SPELL_ATTR_EX3_DEATH_PERSISTENT

            Map* map;
            map = me->GetMap();
            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if( achiev_counter <= 2) {
                        pPlayer->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(5309));// Full of Sound and Fury
                    }

                    if (IsHeroic()) {
                    // because automatic assignment achievement doesn't work correctly,we must do it manually
                        pPlayer->GetAchievementMgr().CompletedAchievement(sAchievementStore.LookupEntry(5115));
                    }
                }
            }

            if(Nef)
            {
                if(IsHeroic())
                {
                    Nef->MonsterYell("Hmm. A shame to lose that experiment...", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(23361, false);
                }
                else
                {
                    Nef->MonsterYell("Impressive! You managed to destroy one of my most horrific creations - a task I'd thought impossible until now.", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(20080, false);

                    if(Creature *Finkle = me->FindNearestCreature(FINKLE, 500, true))
                        Finkle->MonsterYell("Great job guys! The key should be in his guts somewhere. Just, uh, fish around in there 'till you find it.", LANG_UNIVERSAL, 0);
                }

                    Nef->ForcedDespawn(3000);
                    Nef->CastSpell(Nef,87459,true); // Visual teleport
                    Nef = NULL;
            }

            if(Creature *pRobot = me->FindNearestCreature(BILE_O_TRON, 500, true)) // We dont need robot anymore :)
            {
                pRobot->RemoveAllAuras();
            }



            instance->SetData(DATA_CHIMAERON_GUID, DONE);
        }

        void EnterCombat(Unit* /*who*/) 
        {
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 15); // For sure
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAurasDueToSpell(FAST_ASLEEP);
            Nef = me->SummonCreature(NEFARIUS,-113.94f,42.98f,80.2f, 4.7f,TEMPSUMMON_CORPSE_DESPAWN, 1000);
            if(Nef)
            {
                Nef->MonsterYell("Ah, Chimaeron! Truly a living testament to my scientific prowess. I reworked and twisted his form countless times over the years and the final result is truly something to behold.", LANG_UNIVERSAL, 0);
                me->SendPlaySound(23362, false);
                Nef->CastSpell(Nef,87654,true); // Wrong spell -> TODO find correct visual aura
                Nef->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NON_ATTACKABLE);
                Nef->SetReactState(REACT_PASSIVE);
            }

            if (instance)
                instance->SetData(DATA_CHIMAERON_GUID, IN_PROGRESS);
        }

        void EnterEvadeMode()
        {
            if(Creature* pNef = me->FindNearestCreature(NEFARIUS, 500, true) )
            {
                pNef->ForcedDespawn(3000);
                pNef->CastSpell(Nef,87459,true); // Visual teleport
            }

            if(Creature *pRobot = me->FindNearestCreature(BILE_O_TRON, 500, true)) // We dont need robot anymore :)
                pRobot->RemoveAllAuras();

            ScriptedAI::EnterEvadeMode();
        }

        void DamageDealt(Unit* target, uint32& damage, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == DIRECT_DAMAGE) 
                if(me->HasAura(DOUBLE_ATTACK_BUFF))
                {
                    me->RemoveAurasDueToSpell(DOUBLE_ATTACK_BUFF); // odstranim ho
                    me->CastSpell(me,DOUBLE_ATTACK,true); // Seems working
                    //me->AttackerStateUpdate(me->GetVictim());
                    //me->resetAttackTimer();
                    //DoMeleeAttackIfReady(); // Hit again
                }
        }

        void Relax(uint32 feud_on, bool is_heroic)
        {
            me->SetReactState(REACT_PASSIVE);

            if(!feud_on) // Dont casting feud
            {
                Derelax_timer = 4000 + 7000; // 4 second casting time of Massacre
                Slime_timer = 19000;
                Break_timer = 14000;
            }
            else // Casting Feud
            {
                if(is_heroic)
                {
                    Slime_timer = 19000; // First slime after 19 seconds
                    Break_timer = 5000 + 5000;
                    Derelax_timer = 5000 + 5000; // Feud is interrubted by Nef after 5 seconds
                }
                else
                {
                    Derelax_timer = 4000 + 30000 + 7000; // 30 second casting time of feud
                    Slime_timer = 19000; // First slime after 15 seconds
                    Break_timer = 4000 + 30000 + 9000;
                }

                feuding = true;
            }
        }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if(!instance)
            return;

        if(Berserk_timer <= diff) // Hard enrage after 7,5 minutes
        {
            me->CastSpell(me,SPELL_BERSERK,true);
            Berserk_timer = 999999;
        }
        else Berserk_timer -=diff;

        if(me->GetExactDist2d(-106.0f,19.0f)> 60 && me->IsInCombat()) // Evade if we are out the room
            EnterEvadeMode();

    if(PHASE == 1)
    {

            if(Whip_timer <= diff) // Nef interrupt Chim's feud after 5 seconds on HC
            {
                if(whiped)
                    if(IsHeroic())
                    {
                        me->RemoveAurasDueToSpell(DOUBLE_ATTACK_BUFF); // This interrupts casting of Feud and remove double attack if it' active
                        me->InterruptNonMeleeSpells(false);
                        Break_timer = 14000;
                        Whip_timer = 999999;
                    }
                whiped = false;
            }
            else Whip_timer -= diff;

            if(can_feud)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    me->RemoveAurasDueToSpell(DOUBLE_ATTACK_BUFF);

                    DoCast(me,FEUD); // Feud

                    if(Nef)
                    {
                        Nef->MonsterYell("Stop fighting yourself this instant, Chimaeron!", LANG_UNIVERSAL, 0);
                        me->SendPlaySound(23364, false);
                    }

                    if(IsHeroic())
                    {
                        Nef->CastSpell(me,SHADOW_WHIP,false); // Neff s bicikom ? :D :D :D
                        whiped = true;
                        Whip_timer = 4000;
                    }

                    if(Creature *pRobot = me->FindNearestCreature(BILE_O_TRON, 500, true)) // Robot Failure
                    {
                        pRobot->CastSpell(pRobot,SYSTEMS_FAILURE,true);
                        pRobot->RemoveAurasDueToSpell(FINKLE_MIXTURE);
                        pRobot->CastSpell(pRobot,REROUTE_POWER,false);
                    }

                    if(IsHeroic())
                        Massacre_timer = 30000;
                    else
                        Massacre_timer = 1000;

                    can_feud =false;
                }
            }

            if(Derelax_timer <= diff) // Make Chim agressive again,due to small melee delay after feud/massacre
            {
                me->SetReactState(REACT_AGGRESSIVE);
                Derelax_timer = 999999;
            }
            else Derelax_timer -=diff;

            if(Delay_melee_timer <= diff) // Small delay in melee attack after Break, due to Double Attack
            {
                me->SetReactState(REACT_AGGRESSIVE);
                Delay_melee_timer = 999999;
            }
            else Delay_melee_timer -= diff;

            if(Break_timer <= diff)
            {
                if(!me->IsNonMeleeSpellCasted(false)) // Maybe
                {
                    me->CastSpell(me->GetVictim(),BREAK,true);
                    me->CastSpell(me,DOUBLE_ATTACK_BUFF,true);
                    Delay_melee_timer = 7000; // Prevent melee casting for 7 seconds after Double attack
                    me->SetReactState(REACT_PASSIVE);
                    Break_timer = 15000;
                }
            }
            else Break_timer -= diff;

            if(Slime_timer <= diff)
            {
                if(feuding || !me->IsNonMeleeSpellCasted(false))
                {
                    if(urand(0,100) < 20) // Random Finkles yells during encounter
                    {
                        if(Creature* pFinkle = me->FindNearestCreature(FINKLE, 200, true) )
                        {
                            if(urand(0,1))
                                pFinkle->MonsterYell("Poor little fella...", LANG_UNIVERSAL, 0);
                            else
                                pFinkle->MonsterYell("Whoa oh, he looks pretty mad now!", LANG_UNIVERSAL, 0);;
                        }
                    }

                    if (getDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL || getDifficulty() == RAID_DIFFICULTY_25MAN_HEROIC )
                    {
                        uint32 counter = 0;
                        std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
                        for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
                        {
                            counter++;
                            Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());
                            if( unit->ToPlayer() && unit->IsAlive() && counter > 3 && counter <= 8) // Cast slime on 5 targets , excluding tanks ( 3 max )
                                me->CastSpell(unit,CAUSTIC_SLIME_MISSILE,true);
                        }
                    }
                    else // 10 MAN
                    {

                        Unit* target1;
                        if ((target1 = SelectTarget(SELECT_TARGET_RANDOM, 2, 500, true))) // First Caustice slime
                            me->CastSpell(target1,CAUSTIC_SLIME_MISSILE,true);

                        if (Unit* target2 = SelectTarget(SELECT_TARGET_RANDOM, 2, 500, true))// Second Caustic Slime
                            if(target1 != target2)
                                me->CastSpell(target2,CAUSTIC_SLIME_MISSILE,true);
                    }

                    Slime_timer = urand(5000,7000); // Normal casting slime

                    if(feuding) // Casting slime during Feud "phase"
                    {
                        Slime_counter ++;
                        Slime_timer = 5000;
                    }

                    if(Slime_counter == 3) // After 3 slimes in feud phase, add delay and continue normaly
                    {
                        feuding = false;
                        Slime_timer = 20000;
                        Slime_counter = 0;
                    }
                }
            }
            else Slime_timer -= diff;

            if(Massacre_timer <= diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    me->MonsterTextEmote("Chimaeron prepares to massacre his foes !", 0, true);
                    Mass_counter++;

                    if(Mass_counter == 3 ) // 100 % chance after third Massacre
                    {
                        Mass_counter = 0;
                        can_feud = true;
                    }
                    else if (Mass_counter == 2 && urand(0,100) <= 22 ) // 22 % chance after second Massacre ( lucky number :D :P )
                    {
                        Mass_counter = 0;
                        can_feud = true;
                    }

                    DoCast(me,MASSACRE);
                    me->RemoveAurasDueToSpell(DOUBLE_ATTACK_BUFF);
                    Relax(can_feud, IsHeroic()); // Preventing attacking after Massacre for a while
                    Massacre_timer = 30000;
                }
            }
            else Massacre_timer -= diff;


            if(!me->HasReactState(REACT_PASSIVE))
                DoMeleeAttackIfReady();

        if(me->HealthBelowPct(20)) // Switching to PHASE 2
        {
            PHASE = 2;
            Break_timer = 15000;
            Delay_melee_timer = 999999;
            me->CastSpell(me,MORTALITY_HEALING_DEBUFF,true);
            me->CastSpell(me,MORTALITY_BOSS_BUFF,true);
            me->SetReactState(REACT_AGGRESSIVE);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
            me->InterruptNonMeleeSpells(false);
            if( IsHeroic())
            {
                if(Nef)
                {
                    Nef->MonsterYell("This fight has gone a bit stale. Allow me to spice things up!", LANG_UNIVERSAL, 0);
                    me->SendPlaySound(23365, false);
                }

                me->CastSpell(me,MOCKING_SHADOWS,true);
            }
        }
    }

        if(PHASE == 2)
        {
            if(Break_timer <= diff)
            {
                me->CastSpell(me->GetVictim(),BREAK,true);
                me->CastSpell(me,DOUBLE_ATTACK_BUFF,true);
                Delay_melee_timer = 5000; // Prevent melee casting for 5 seconds after Double attack
                Break_timer = 15000;
                me->SetReactState(REACT_PASSIVE);
            }
            else Break_timer -= diff;

            if(Delay_melee_timer <= diff)
            {
                me->ClearUnitState(UNIT_STATE_CASTING);
                me->SetReactState(REACT_AGGRESSIVE);
                Delay_melee_timer = 999999;
            }
            else Delay_melee_timer -= diff;

            DoMeleeAttackIfReady();

        }
    }
    };
};


/******************** BILE_O_TRON  ****************************/
class Bile_o_tron_robot : public CreatureScript
{
public:
    Bile_o_tron_robot() : CreatureScript("Bile_o_tron_robot") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Bile_o_tron_robotAI (creature);
    }

    struct Bile_o_tron_robotAI : public ScriptedAI
    {
        Bile_o_tron_robotAI(Creature* creature) : ScriptedAI(creature) {}

        bool removed;
        bool applied;
        uint32 Remove_timer;

        void Reset()
        {
            removed = applied = false;
        }

        void UpdateAI(const uint32 diff)
        {
           if(!me->HasAura(SYSTEMS_FAILURE))
                Remove_timer = 999999;


            if(me->HasAura(SYSTEMS_FAILURE) && !removed)
            {
                Remove_timer = 26000;
                removed = true;
            }

            if(Remove_timer <=diff)
            {
                me->RemoveAurasDueToSpell(SYSTEMS_FAILURE);
                DoCast(me,FINKLE_MIXTURE,true);
                me->AddAura(FINKLE_MIXTURE,me); // Sometimes it gets bugged
                Remove_timer = 999999;
                removed = false;
            }
            else Remove_timer-= diff;

        }
    };
};


#define GOSSIP_ITEM_FINKLE_1    "I suppose you'll be needing a key for this cage? Wait, don't tell me. The horrific gibbering monster behind me ate it, right?"
#define GOSSIP_ITEM_FINKLE_2    "You were trapped, as I recall. This situation seems oddly similar."
#define GOSSIP_ITEM_FINKLE_3    "Gnomes in Lava Suits, for example."
#define GOSSIP_ITEM_FINKLE_4    "No, I, uh, haven't seen it. You were saying?"
#define GOSSIP_ITEM_FINKLE_5    "Restrictions? What restrictions?"


class Finkle_Einhorn_npc : public CreatureScript
{
public:
    Finkle_Einhorn_npc() : CreatureScript("Finkle_Einhorn_npc") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*Sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FINKLE_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(90002, creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FINKLE_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(90003, creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FINKLE_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
            player->SEND_GOSSIP_MENU(90004, creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FINKLE_5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
            player->SEND_GOSSIP_MENU(90005, creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            player->SEND_GOSSIP_MENU(90006, creature->GetGUID());

            if (Creature* pChim = GetClosestCreatureWithEntry(creature, CHIMAERON, 200.0f))
            {
                pChim->SetInCombatWithZone();
                pChim->SetReactState(REACT_AGGRESSIVE);
                pChim->RemoveAurasDueToSpell(FAST_ASLEEP);
            }
            else
                break;

            if (Creature* pRobot = GetClosestCreatureWithEntry(creature, BILE_O_TRON, 500.0f))
            {
                pRobot->CastSpell(pRobot,FINKLE_MIXTURE,true);
                pRobot->AddAura(FINKLE_MIXTURE,pRobot); // Sometimes it gets bugged
                pRobot->CastSpell(pRobot,FINKLE_MIXTURE_SPRAY,true);
                pRobot->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            }
            break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if(player->GetInstanceScript())
        {
            if(player->GetInstanceScript()->GetData(DATA_CHIMAERON_GUID) == DONE || player->GetInstanceScript()->GetData(DATA_CHIMAERON_GUID) == IN_PROGRESS)
                return false; // Event cannot be started during fight or after defeating Chimaeron
        }
        else // If not in instance
            return false;

        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_FINKLE_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        player->SEND_GOSSIP_MENU(90001, creature->GetGUID());

        return true;
    }
};

//Finkle's Mixture 
class spell_gen_finkles_mixture : public SpellScriptLoader
{
    public: spell_gen_finkles_mixture() : SpellScriptLoader("spell_gen_finkles_mixture") { }

        class spell_gen_finkles_mixture_AuraScript : public AuraScript 
        {
                PrepareAuraScript(spell_gen_finkles_mixture_AuraScript);

                bool Load()
                {
                    return true;
                }

                void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
                {
                    amount = -1;
                }

                void Absorb(AuraEffect * /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
                {
                    if (GetTarget()->GetHealth() > 10000 && dmgInfo.GetDamage() >= GetTarget()->GetHealth())
                        absorbAmount = dmgInfo.GetDamage() - GetTarget()->GetHealth() + 1;
                }

                void Register()
                {
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_finkles_mixture_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
                    OnEffectAbsorb += AuraEffectAbsorbFn(spell_gen_finkles_mixture_AuraScript::Absorb, EFFECT_1);
                }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_gen_finkles_mixture_AuraScript();
        }
};

void AddSC_boss_chimaeron()
{
    new boss_chimaeron();
    new Bile_o_tron_robot();
    new Finkle_Einhorn_npc();
    new spell_gen_finkles_mixture();
}
