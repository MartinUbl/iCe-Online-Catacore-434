/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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



enum Spells
{
/******* FELUDIUS ABILITIES *******/
    SPELL_FROZEN_BLOOD = 92505,
    SPELL_HEART_OF_ICE = 82665,
    SPELL_WATER_BOMB   = 82699,
    SPELL_WATER_B_DMG  = 82700,
    SPELL_HYDRO_LANCE  = 82752,
    SPELL_WATER_LOGGED = 82762,
    SPELL_FROST_IMBUED = 82666, // buff na zvyseny dmg do Ignaca po zasiahnuti water bombou
    SPELL_GLACIATE     = 82746,
    SPELL_FROZEN       = 92505, // pri zasahu glaciate hrac dostane  "kocku"/"flash freeze"

/******* IGNACIOUS ABILITIES *******/
    SPELL_RISING_FLAMES     = 82636,
    SPELL_FLAME_TORRENT     = 82777,
    SPELL_AEGIS_OF_FLAME    = 82631,
    SPELL_RISING_FLAMES_BUFF= 82639,
    SPELL_INFERNO_RUSH      = 88579, // HEROIC LEAP ZATIAL 94954
    SPELL_BURNING_BLOOD     = 82660,

/******* BUNNY NPC ABILITIES *******/
   SPELL_FLAME_PATCH        = 70415,
   SPELL_INFERNO_RUSH_AOE   = 88579,

};
/*********************************************** DALSIE ABILITY SOM BOL UZ LENIVY PISAT CEZ ENUMU :D **************************************************/
enum Creatures
{
    IGNACIOUS_ENTRY        = 43686,
    FELUDIUS_ENTRY         = 43687,
    ARION_ENTRY            = 43688,
    TERRASTRA_ENTRY        = 43689,
    CREATURE_FIRE_PATCH    = 48711,
    CREATURE_WATER_BOMB    = 44201,
    CREATURE_GRAVITY_WELL  = 44824,
    CREATURE_ERUPTION_NPC  = 44845,
    CREATURE_LAVA_SEED     = 919337,
    CREATURE_INSTABILITY   = 919338,
    CREATURE_LIQUID_ICE    = 919339,
};

const Position Tele_pos[4] =              // Random position for teleport Arion after he cast Disperse
{
    {-967.409f, -612.65f, 831.92f, 2.41f},
    {-968.11f, -551.05f, 831.92f, 4.22f},
    {-1012.76f, -538.97f, 831.92f, 5.0f},
    {-1037.94f, -589.27f, 831.92f, 0.17f},
};

const Position Gravity_pos[6] =           // Random position for spawn Gravity Well
{
    {-981.3f, -555.3f, 831.92f, 0.0f},
    {-970.8f, -592.2f, 831.92f, 0.0f},
    {-1013.1f, -561.25f, 831.92f, 0.0f},
    {-999.6f, -6600.0f, 831.92f, 0.0f},
    {-1037.1f, -613.77f, 835.29f, 0.0f},
    {-1044.19f, -562.2f, 835.2f, 0.0f},
};

const Position Seeds_pos[16] =  // Pozicie na spawn lava seedov :D Ostatne po miestnosti robim algoritmom :)
{
    {-1029.89f,-625.49f, 834.75f, 0.0f},
    {-1040.4f, -615.1f, 835.21f, 0.0f},
    {-1055.87f, -614.1f, 835.1f, 0.0f},
    {-1063.5f, -588.81f, 835.21f, 0.0f},
    {-1052.94f, -579.16f, 835.1f, 0.0f},
    {-1053.47f, -564.51f, 835.21f, 0.0f},
    {-1053.28f, -549.25f, 835.05f, 0.0f},
    {-1042.24f, -537.269f,835.05f, 0.0f},
    {-1031.18f, -546.36f, 835.28f, 0.0f},
    {-1086.95f, -588.06f, 841.3f, 0.0f},
    {-1087.71f, -577.06f, 841.3f, 0.0f},
    {-1099.57f, -583.00f, 841.3f, 0.0f},
    {-1068.9f, -590.992f, 837.38f, 0.0f},
    {-1067.31f, -572.6f,  836.9f, 0.0f},
    {-1028.28f, -544.1f, 834.1f, 0.0f},
    {-918.17f, -583.05f, 831.91f, 0.0f},
};
/************************* ASCENDANT COUNCIL *******************************/
class boss_Elementium_Monstrosity : public CreatureScript
{
public:
    boss_Elementium_Monstrosity() : CreatureScript("boss_Elementium_Monstrosity") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_Elementium_MonstrosityAI (creature);
    }

    struct boss_Elementium_MonstrosityAI : public ScriptedAI
    {
        boss_Elementium_MonstrosityAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Gravity_crush_timer;
        uint32 LiquidIce_timer;
        uint32 Lava_seed_timer;
        uint32 Lava_spell_timer;
        uint32 Instability_timer;
        uint32 Intensity_timer; // cca 20s sekund sa prida frekvencia Instability
        uint32 spawn_timer;
        uint32 INSTABILITY;
        bool killed_unit,spawned,can_seed,castingGC;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            spawn_timer=5000; // Prve 3 sekundy po spawne by mal byt pasivny ( som milosrdny 5 :D )
            Gravity_crush_timer=spawn_timer+28000;
            LiquidIce_timer=spawn_timer+4000;
            Lava_spell_timer=19000; // 2 sekundy po caste sa spawnu "lava seeds"
            Instability_timer=5000;
            Intensity_timer=20000; // kazdych 20s sa zvysi frekvencia instability
            INSTABILITY=1;
            killed_unit=spawned=can_seed=castingGC=false;
            DoCast(me,84918); // Cryogenic aura
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->MonsterYell("BEHOLD YOUR DOOM!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20396, false);
        }

        void KilledUnit(Unit * /*victim*/)
        {
            if(killed_unit==false)
            {
                me->MonsterYell("Annihilate....", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20397, false);
                killed_unit=true;
            }
            else
            {
                me->MonsterYell("Eradicate....", LANG_UNIVERSAL, NULL);
                me->SendPlaySound(20398, false);
                killed_unit=false;
            }
        }

        void JustDied (Unit * killed)
        {
            me->MonsterYell("Impossible....", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20399, false);
        }


        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if(Lava_spell_timer<=diff) // Visual cast lava seed
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    DoCast(84913); // Visual cast lava seed
                    me->addUnitState(UNIT_STAT_CASTING); // Chcem si byt isty :D
                    Lava_seed_timer=2000; // Hned po docasteni spawnem lava seeds
                    Lava_spell_timer=23000;
                    can_seed=true;
                }
            }
            else Lava_spell_timer-=diff;


            if(Lava_seed_timer<=diff && can_seed)
            {
                me->clearUnitState(UNIT_STAT_CASTING);
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_seed=false;
                    Instability_timer=6000; // Musim takto instability bunny je nastaveny na 6s
                    float uhol=0.0f;
                    float kopia_uhol=0.0f;
                    float dlzka=0.0f;
                    int pocet_flamov=0;

                for(int i=1;i<17;i++)
                {
                    float fall_distance=me->GetDistance(Seeds_pos[i].GetPositionX(),Seeds_pos[i].GetPositionY(),Seeds_pos[i].GetPositionZ());
                    //Creature* seed=me->SummonCreature(CREATURE_LAVA_SEED,Seeds_pos[i].GetPositionX(),Seeds_pos[i].GetPositionY(),Seeds_pos[i].GetPositionZ()+fall_distance/2,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    //seed->GetMotionMaster()->MoveFall(0);
                    Creature* seed=me->SummonCreature(CREATURE_LAVA_SEED,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()+6,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    seed->SetFlying(true);
                    seed->GetMotionMaster()->MoveJump(Seeds_pos[i].GetPositionX(),Seeds_pos[i].GetPositionY(),Seeds_pos[i].GetPositionZ(),25.0f,fall_distance/2.0);
                    seed->SetFlying(false);
                }
                for(int i=0;i<6;i++)
                {
                    pocet_flamov+=4;
                    uhol=(((360 / pocet_flamov)*3.14)/180);
                    dlzka=dlzka+11.2f; // polomer kruznice

                    for(int j=0;j<pocet_flamov;j++)
                    {
                        kopia_uhol+=uhol;
                        float fall_distance=me->GetDistance(-1009.1f+cos(kopia_uhol)*dlzka,-582.5f+sin(kopia_uhol)*dlzka,831.91f);
                        Creature* seed=me->SummonCreature(CREATURE_LAVA_SEED,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()+6,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                        seed->SetFlying(true);
                        seed->GetMotionMaster()->MoveJump(-1009.1f+cos(kopia_uhol)*dlzka,-582.5f+sin(kopia_uhol)*dlzka,831.91f,25.0f,fall_distance/2.0);
                        seed->SetFlying(false);
                    }
                }
                        // Jeden seed spawn do stredu miestnosti
                        me->SummonCreature(CREATURE_LAVA_SEED,-1009.1f,-582.5f,831.91f, 0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,30000);
                }
            }
            else Lava_seed_timer-=diff;

            if(spawn_timer<=diff && !spawned) // Na zaciatku ma byt monstrosity par sekund v "klude"
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                me->SetReactState(REACT_AGGRESSIVE);
                spawned=true;
            }
            else spawn_timer-=diff;

            if(Intensity_timer<=diff) // Kazdych 20 sekund sa zvysi pocet instability "castov" o 1
            {
                    INSTABILITY=INSTABILITY+1;
                    Intensity_timer=20000;
            }
            else Intensity_timer-=diff;

            if(LiquidIce_timer<=diff) // Kazde 2.5s Liquide ice patch pod seba
            {
                    me->SummonCreature(CREATURE_LIQUID_ICE,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    LiquidIce_timer=2500;
            }
            else LiquidIce_timer-=diff;

            if(Gravity_crush_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    {
                        castingGC=true;
                        target->ToPlayer()->GetMotionMaster()->MoveJump(me->getVictim()->GetPositionX(),me->getVictim()->GetPositionY(),me->getVictim()->GetPositionZ()+30,5.0f,0.8f);
                        DoCast(target,84948);
                        me->MonsterYell("FEEL THE POWER!", LANG_UNIVERSAL, NULL);
                        me->SendPlaySound(20400, false);
                        Gravity_crush_timer=25000;
                    }
                }
            }
            else Gravity_crush_timer-=diff;
/////////////////////////////////////////////////////////////////////////////////////////////////
                        if(Instability_timer<=diff)
                        {
                            uint32 number_of_players=0;

                            if(!castingGC) //Ak castim Gravity crush bunny bude davat dmg za bossa po dobu 6 sekund
                            {
                                castingGC=false;
                                std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
                                for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
                                {
                                    Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());
                                    if ( (unit->GetTypeId() == TYPEID_PLAYER) && unit->isAlive() )
                                        number_of_players++;
                                }
                                    Unit** pole;
                                    pole=(Unit**)malloc(number_of_players*sizeof(Unit*));

                                    uint32 iter=0;
                                    for (i = me->getThreatManager().getThreatList().begin(); i!= me->getThreatManager().getThreatList().end(); ++i)
                                    {
                                        Unit* unit = Unit::GetUnit(*me, (*i)->getUnitGuid());
                                        if ( (unit->GetTypeId() == TYPEID_PLAYER) && unit->isAlive() )
                                        {
                                            pole[iter]=unit; // zapisem si hracov do dynamickeho pola
                                            iter++;
                                        }
                                    }

                                    Unit* pom=NULL;
                                    for(uint32 i=0; i<number_of_players-1;i++)
                                    for(uint32 j=0; j<number_of_players-1;j++)
                                        if(pole[j]->GetDistance(me) < pole[j+1]->GetDistance(me))
                                        {
                                            pom=pole[j];
                                            pole[j]=pole[j+1];
                                            pole[j+1]=pom;
                                        }

                                    for(uint32 i=0;i<INSTABILITY;i++)
                                    {
                                        if(i<number_of_players)
                                        {
                                            DoCast(pole[i],84529);
                                            //me->MonsterYell("Castim cez pole hracov!", LANG_UNIVERSAL, NULL);
                                        }
                                        else if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true)) // Keby nahodou
                                                DoCast(target,84529);

                                    } // kedze nejde zastupi tuto funkciu nachvilu bunny :D
                                    Instability_timer=1000;
                                    free((void*)pole);
                                }
                            else
                            {
                                me->SummonCreature(CREATURE_INSTABILITY,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ()+6,0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                                Instability_timer=6000;
                            }
                        }
                        else Instability_timer-=diff;
            DoMeleeAttackIfReady();
        }
    };
};

class mob_lava_seed : public CreatureScript
{
public:
    mob_lava_seed() : CreatureScript("mob_lava_seed") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_lava_seedAI (creature);
    }

    struct mob_lava_seedAI : public ScriptedAI
    {
        mob_lava_seedAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Lava_erupt_timer;
        uint32 Rand_eruption_timer; // Ak sa mi nepodari najst cas erupcie cez dve riesenia stanovym random time explozie
        bool erupted,found_position;

        void Reset()
        {
            Lava_erupt_timer=99999;
            Rand_eruption_timer=8000; // Ak sa mi do 8 sekund nepoadri najst speavny cas -> nastavim random
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
            erupted=found_position=false;
            DoCast(me,84911);
            me->ForcedDespawn(15000); // Keby nahodou
        }

        void JustDied (Unit * killed)
        {
            me->setDeathState(DEAD);
            me->RemoveCorpse();
        }

        void UpdateAI(const uint32 diff)
        {
            if(!found_position) // Ak som este nenasiel moju poziciu po dopade
            {
                for(int i=1;i<17;i++) // Prekontrolujem 17 pevnych spawn pozicii
                    {
                        if(me->GetPositionX()==Seeds_pos[i].GetPositionX() && me->GetPositionY()==Seeds_pos[i].GetPositionY())
                        {
                            if(Creature *pMonstr = me->FindNearestCreature(43735, 500, true))
                            {
                                Lava_erupt_timer=2000+uint32(me->GetDistance2d(pMonstr)/20)*1000;
                                found_position=true;
                            }
                        }
                    }
             }

             if(!found_position)
             {
                if(me->GetPositionZ()==831.91f)
                {
                    if(Creature *pMonstr = me->FindNearestCreature(43735, 500, true))
                            {
                                Lava_erupt_timer=2000+uint32(me->GetDistance2d(pMonstr)/20)*1000;
                                found_position=true;
                            }
                }
             }

            if(Lava_erupt_timer<=diff && !erupted)
            {
                DoCast(me,84912); // dmg + visual eruption od lava
                erupted=true;
                me->ForcedDespawn(2000);
            }
            else Lava_erupt_timer-=diff;

            if(Rand_eruption_timer<=diff && !found_position) // Nepodarilo sa najst spravny eruption timer nastavim random 
            {
                Lava_erupt_timer=urand(2000,3500);
                found_position=true;
            }
            else Rand_eruption_timer-=diff;

        }

            };
};

class mob_liquide_ice : public CreatureScript
{
public:
    mob_liquide_ice() : CreatureScript("mob_liquide_ice") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_liquide_iceAI (creature);
    }

    struct mob_liquide_iceAI : public ScriptedAI
    {
        mob_liquide_iceAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Liquid_timer;
        uint32 Growing_timer;
        bool buffed;

        void Reset()
        {
            buffed=false;
            Liquid_timer=200;
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
            Growing_timer= 2000;
        }

        void UpdateAI(const uint32 diff)
        {
            if(Creature *pMonstr = me->FindNearestCreature(43735, 500, true))
            {
                if(!pMonstr->isInCombat() ) // Ak je Monstrosity mrtva alebo nie je v combate despawnem liquid ice plosku
                me->ForcedDespawn();
            }
            else me->ForcedDespawn();

            if(Liquid_timer<=diff && !buffed)
            {
                DoCast(me,84917); // zvacsi plosku
                DoCast(me,84914); // dmg + visual liquid ice
                buffed=true;
            }
            else Liquid_timer-=diff;

            if(Growing_timer<=diff)
            {
                if(Creature *pMonstr = me->FindNearestCreature(43735, 500, true))
                {
                    if(me->IsWithinMeleeRange(pMonstr))
                        DoCast(me,84917); // Zvacsenie plosky avsak radius nedokazem dynamicky menit
                        Growing_timer=2000;
                }
            }
            else Growing_timer-=diff;
        }

            };
};

class instability_bunny : public CreatureScript
{
public:
    instability_bunny() : CreatureScript("instability_bunny") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new instability_bunnyAI (creature);
    }

    struct instability_bunnyAI : public ScriptedAI
    {
        instability_bunnyAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Instability_timer;
        Creature *pMonstrosity;

        void Reset()
        {
            me->SetFlying(true);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_NOT_SELECTABLE);
            Instability_timer=500;
            me->SetInCombatWithZone();
            pMonstrosity=NULL;
            me->ForcedDespawn(6000);
        }

        void JustDied (Unit * killed)
        {
            me->setDeathState(DEAD);
            me->RemoveCorpse();
        }

        void UpdateAI(const uint32 diff)
        {
            Creature * pMonstrosity; // Ak nenajdem
            if(!(pMonstrosity = (me->FindNearestCreature(43735, 500, true))) || pMonstrosity->isDead())
                 me->ForcedDespawn();

            if(Instability_timer<=diff)
            {
                if(pMonstrosity)
                    me->NearTeleportTo(pMonstrosity->GetPositionX(),pMonstrosity->GetPositionY(),pMonstrosity->GetPositionZ()+6,0.0f);
                for(int i=0;i<3;i++)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 500, true))
                    {
                            if((pMonstrosity) || (pMonstrosity = me->FindNearestCreature(43735, 500, true)))
                            {
                                me->NearTeleportTo(pMonstrosity->GetPositionX(),pMonstrosity->GetPositionY(),pMonstrosity->GetPositionZ()+6,0.0f);
                                DoCast(target,84529); // Instability dmg
                            }
                            me->SetInCombatWithZone();
                    }
                }
                Instability_timer=1000;
            }
            else Instability_timer-=diff;
        }

            };
};


/************************* FELUDIUS *******************************/
class mob_felo : public CreatureScript
{
public:
    mob_felo() : CreatureScript("mob_felo") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_feloAI (creature);
    }

    struct mob_feloAI : public ScriptedAI
    {
        mob_feloAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Heart_debuff_timer;
        uint32 Water_bomb_timer;
        uint32 Hydrolance_timer;
        uint32 Glaciate_timer;
        uint32 Frozen_timer;
        uint32 Tele_debug_timer;
        uint32 PHASE;
        bool check_debuff,Hp_dropped,can_interrupt,debuged;


        void Reset()
        {
            me->SetVisible(true);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE); // na test
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            Heart_debuff_timer=17000;
            Water_bomb_timer=15000;
            Hydrolance_timer=8000;
            Glaciate_timer=30000;
            Frozen_timer=3000;
            check_debuff=Hp_dropped=can_interrupt=false;
            debuged=true;
            PHASE=1;
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(int i=0;i<3;i++)
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(can_interrupt)
                    {
                        me->InterruptNonMeleeSpells(true);
                        break;
                    }
                }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->MonsterYell("You dare invade our lord's sanctum?", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20162, false);

            if(Creature *pIgnacious = me->FindNearestCreature(IGNACIOUS_ENTRY, 300, true))
            {
                pIgnacious->SetInCombatWithZone();
                if(!pIgnacious->isInCombat()) // Ak neni Feludius v combate donutim ho :D
                {
                    if(!pIgnacious->IsInEvadeMode())
                    {
                        pIgnacious->Attack(me->getVictim(),true);
                        pIgnacious->GetMotionMaster()->MoveChase(me->getVictim()); // zostal na mieste ;)
                    }
                }
            }
        }

        void KilledUnit(Unit * /*victim*/)
        {
            me->MonsterYell("Perish!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20163, false);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                    return;

                if(PHASE==1)
                {
                if(Creature *pIgnacious = me->FindNearestCreature(IGNACIOUS_ENTRY, 300, true)) // Musel som to dat aj sem pretoze ked je jeden z bossov v evademode tak ignoruje vsetko, t.j dalo by sa to bugovat
                {
                    if(!pIgnacious->isInCombat()) // Ak neni Feludius v combate donutim ho :D
                    {
                        if(!pIgnacious->IsInEvadeMode())
                        {
                            pIgnacious->Attack(me->getVictim(),true);
                            me->SetInCombatWithZone();
                            pIgnacious->GetMotionMaster()->MoveChase(me->getVictim());
                        }
                    }
                }

                if(Frozen_timer<=diff && check_debuff)
                {
                    check_debuff=false;
                    std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                        for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                        {
                            Unit* target = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                            if (target && target->GetTypeId() == TYPEID_PLAYER && target->isAlive())
                                if(target->HasAura(SPELL_WATER_LOGGED))
                                DoCast(target,SPELL_FROZEN); // Ak ma target na sebe watterlogged debuff dostane FROZEN
                        }

                }
                else Frozen_timer-=diff;

                if(Water_bomb_timer<=diff)
                {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    DoCast(SPELL_WATER_BOMB); // pure visual

                    std::list<HostileReference*> t_list = me->getThreatManager().getThreatList();
                        for (std::list<HostileReference*>::const_iterator itr = t_list.begin(); itr!= t_list.end(); ++itr)
                        {
                            Unit* target = Unit::GetUnit(*me, (*itr)->getUnitGuid());

                            if (target && target->GetTypeId() == TYPEID_PLAYER )
                            {
                                me->SummonCreature(CREATURE_WATER_BOMB,target->GetPositionX()+urand(0,10)-urand(0,10),target->GetPositionY()+urand(0,10)-urand(0,10),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                                if (urand(0,1)) // +par bomb naviac cca 5
                                me->SummonCreature(CREATURE_WATER_BOMB,target->GetPositionX()+urand(0,5)-urand(0,5),target->GetPositionY()+urand(0,5)-urand(0,5),target->GetPositionZ(),0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                            }
                        }
                        Water_bomb_timer=32000;
                 }
                }
                else Water_bomb_timer-=diff;

                    if(Hydrolance_timer<=diff)
                    {
                        if(!me->IsNonMeleeSpellCasted(false))
                        {
                            can_interrupt=true;
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                                DoCast(target,SPELL_HYDRO_LANCE);
                            Hydrolance_timer=urand(10000,40000);
                        }
                    }
                    else Hydrolance_timer-=diff;

                        if(Glaciate_timer<=diff)
                        {
                            if(!me->IsNonMeleeSpellCasted(false))
                            {
                                can_interrupt=false;
                                check_debuff=true;
                                Frozen_timer=3000; // po docasteni kontrolujem ci ma na sebe niekto watterlogged debuff
                                DoCast(me,SPELL_GLACIATE);
                                Glaciate_timer=32000;
                                me->MonsterYell("I will freeze the blood in your veins!", LANG_UNIVERSAL, NULL);
                                me->SendPlaySound(20164, false);
                            }
                        }
                        else Glaciate_timer-=diff;

                            if(Heart_debuff_timer<=diff && !me->IsNonMeleeSpellCasted(false))
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                                    DoCast(target,SPELL_HEART_OF_ICE);
                                Heart_debuff_timer=17000;
                            }
                            else Heart_debuff_timer-=diff;

                            if(Creature* pIgnacious = me->FindNearestCreature(IGNACIOUS_ENTRY, 500, true) )
                                {
                                    if(pIgnacious->HealthBelowPct(25))
                                        PHASE=2;
                                }

                            DoMeleeAttackIfReady();
                     }

                     if(Tele_debug_timer<=diff && !debuged)
                     {
                        me->NearTeleportTo(-1044.48f,-551.18f,835.5f, 0.98f);
                        debuged=true;
                     }
                     else Tele_debug_timer-=diff;

                            if(PHASE==2 || (HealthBelowPct(25) && !Hp_dropped))
                            {
                                Tele_debug_timer=500; // Buguje sa teleport musim ho volat znova pre istotu
                                debuged=false;
                                me->InterruptNonMeleeSpells(true);
                                DoCast(me,87459); // Visual teleport
                                me->NearTeleportTo(-1044.48f,-551.18f,835.15f, 0.98f);
                                PHASE=3;
                                me->RemoveAllAuras();
                                me->SetReactState(REACT_PASSIVE);
                                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                                me->AttackStop();
                                me->NearTeleportTo(-1044.48f,-551.18f,835.15f, 0.98f);
                                DoCast(me,83810); // Visual Water channel
                                me->SummonCreature(ARION_ENTRY,-1051.28f,-599.69f,835.21f, 5.7f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                                me->NearTeleportTo(-1044.48f,-551.18f,835.15f, 0.98f);
                                Hp_dropped=true;
                            }

                            if(PHASE==3) //  Zakamuflujem sa a portnem sa na spawn position ak sa spawnla Monstrosity
                            {
                                if(me->FindNearestCreature(ARION_ENTRY, 500, true) == NULL)
                                {
                                    me->SetVisibility(VISIBILITY_OFF);
                                    me->NearTeleportTo(-1044.48f,-551.18f,835.15f, 0.98f);
                                    PHASE=4;
                                }
                            }
        }
    };
};

/************************* IGNACIOUS *******************************/
class mob_ignac : public CreatureScript
{
public:
    mob_ignac() : CreatureScript("mob_ignac") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_ignacAI (creature);
    }

    struct mob_ignacAI : public ScriptedAI
    {
        mob_ignacAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Rising_flames_timer;
        uint32 Flame_torrent_timer;
        uint32 Inferno_rush_timer;
        uint32 Burning_blood_timer;
        uint32 Ticking_timer;
        uint32 spread_flame_timer;
        uint32 Tele_debug_timer;
        uint32 PHASE;
        uint32 flames_break;
        uint32 Speaking_timer;
        uint32 Stack,counter;
        bool aegis_used,ticked,can_spread_fire,can_knock,Hp_dropped,speaked,can_interrupt,debuged;
        Unit* Rush_target;



        void Reset()
        {
            PHASE=1;
            Speaking_timer=4000;
            me->SetVisible(true);
            me->SetSpeed(MOVE_RUN,1.5f,true);
            me->SetReactState(REACT_AGGRESSIVE);
            flames_break=0;
            me->clearUnitState(UNIT_STAT_CASTING); // keby nahodou
            Rising_flames_timer=33000;
            Flame_torrent_timer=10000;
            Inferno_rush_timer=15000;
            Burning_blood_timer=27000;
            Ticking_timer=1000;
            spread_flame_timer=100;
            Stack=counter=0;
            aegis_used=ticked=can_spread_fire=can_knock=Hp_dropped=speaked=can_interrupt=false;
            debuged=true;
            Rush_target=NULL;
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(int i=0;i<3;i++)
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(can_interrupt)
                    {
                        if(!me->HasAura(SPELL_AEGIS_OF_FLAME))
                            me->InterruptNonMeleeSpells(true);
                        break;
                    }
                }
        }

        void KilledUnit(Unit * /*victim*/)
        {
            me->MonsterYell("More fuel for the fire!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20286, false);
        }


        void EnterCombat(Unit* /*who*/)
        {
            if(Creature *pFeludius = me->FindNearestCreature(FELUDIUS_ENTRY, 300, true))
            {
                pFeludius->SetInCombatWithZone();
                if(!pFeludius->isInCombat()) // Ak neni Feludius v combate donutim ho :D
                {
                    if(!pFeludius->IsInEvadeMode())
                    {
                        pFeludius->Attack(me->getVictim(),true);
                        pFeludius->GetMotionMaster()->MoveChase(me->getVictim()); // zostal na mieste dakedy ;)
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

                    if(Speaking_timer<=diff && !speaked)
                    {
                        me->MonsterYell("You will die for your insolence!", LANG_UNIVERSAL, NULL);
                        me->SendPlaySound(20285, false);
                        speaked=true;
                    }
                    else Speaking_timer-=diff;
    if(PHASE==1)
    {
            if(Creature *pFeludius = me->FindNearestCreature(FELUDIUS_ENTRY, 300, true)) // Musel som to dat aj sem pretoze ked je jeden z bossov v evademode tak ignoruje vsetko, t.j dalo by sa to bugovat
            {
                if(!pFeludius->isInCombat())
                {
                    if(!pFeludius->IsInEvadeMode())
                    {
                        me->SetInCombatWithZone();
                        pFeludius->Attack(me->getVictim(),true);
                        pFeludius->GetMotionMaster()->MoveChase(me->getVictim());
                    }
                }
            }


            if(me->HasAura(SPELL_AEGIS_OF_FLAME) && aegis_used && !me->IsNonMeleeSpellCasted(false)) //Neslo inak - cast rising flames hned po nahodeni stitu
            {
                can_interrupt=true;
                me->CastSpell(me,SPELL_RISING_FLAMES, true);
                aegis_used=false;
            }

            if(Rising_flames_timer<=diff) // Cast stit na seba
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                   can_interrupt=false;
                   Stack=counter=0;
                   DoCast(me,SPELL_AEGIS_OF_FLAME);
                   aegis_used=true;
                   Rising_flames_timer=60000;
                   me->MonsterYell("BURN!", LANG_UNIVERSAL, NULL);
                   me->SendPlaySound(20287, false);
                }
            }
            else Rising_flames_timer-=diff;

            if(me->HasAura(SPELL_AEGIS_OF_FLAME))
            {
                if(!ticked)
                {
                    Ticking_timer=diff;
                    ticked=true;
                }
                    if(Ticking_timer<=diff)// Kazde 2s ak mam na sebe stit zvysim dmg o 5% (jeden stack)
                    {
                        counter++;
                        Stack=Stack+counter; // Podla Tankspotu by mala byt postupnost 1 3 6 10 .... stackov
                        for(uint32 i=0;i<(Stack-me->GetAuraCount(SPELL_RISING_FLAMES_BUFF));i++) // Pocet stackov sa zvysuje s kazdym tiknutim
                            me->AddAura(SPELL_RISING_FLAMES_BUFF,me);

                            if( (me->GetAuraCount(SPELL_RISING_FLAMES_BUFF))>1)
                                me->RemoveAuraFromStack(SPELL_RISING_FLAMES_BUFF);

                        Ticking_timer=2000;
                    }
                    else Ticking_timer-=diff;
            }

                if(Flame_torrent_timer<=diff )
                {
                    if(!me->IsNonMeleeSpellCasted(false))
                    {
                        me->getVictim()->DealDamage(me->getVictim(),500000, NULL, SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NATURE,false);
                        can_interrupt=false;
                        DoCast(me->getVictim(),SPELL_FLAME_TORRENT);
                        Flame_torrent_timer=urand(13000,20000);
                        ticked=false; // urcite vycasti do 60 aspon jeden torrent
                    }
                }
                else Flame_torrent_timer-=diff;

                    if(Burning_blood_timer<=diff )
                    {
                        if(!me->IsNonMeleeSpellCasted(false))
                        {
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true))
                                 me->CastSpell(target,SPELL_BURNING_BLOOD,true);

                            Burning_blood_timer=27000;
                        }
                    }
                    else Burning_blood_timer-=diff;

                        if(Inferno_rush_timer<=diff ) // (1) Rozbehnem sa za najvzdialenejsim hracom 
                        {
                            if(!me->IsNonMeleeSpellCasted(false))
                            {
                                Rush_target=NULL;
                                if ((Rush_target=SelectTarget(SELECT_TARGET_FARTHEST, 0,50, true)) != NULL)
                                {
                                    float uhol=me->GetAngle(Rush_target->GetPositionX(), Rush_target->GetPositionY());
                                    float dlzka=me->GetDistance(Rush_target); // vzdialenost medzi hracom a bossom
                                    float jednotkova_dlzka=0.0f;

                                    // Jeden flame pod seba
                                    me->SummonCreature(CREATURE_FIRE_PATCH,me->GetPositionX(),me->GetPositionY(),842.0f, 0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,30000);

                                    // Ostatne po urcitym uhlom ku hracovi
                                    for(int i=0;i<int(dlzka/5);i++)
                                    {
                                        jednotkova_dlzka=jednotkova_dlzka+5.0f; // polomer kruznice
                                        me->SummonCreature(CREATURE_FIRE_PATCH,me->GetPositionX()+cos(uhol)*jednotkova_dlzka,me->GetPositionY()+sin(uhol)*jednotkova_dlzka,842.0f, 0.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,30000);
                                    }

                                    me->addUnitState(UNIT_STAT_CASTING); // nechceme aby zacal castit ked bude bezat znova za tankom
                                    me->GetMotionMaster()->MoveCharge(Rush_target->GetPositionX(),Rush_target->GetPositionY(),Rush_target->GetPositionZ(),42.0f);
                                    me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                                    can_knock=true;
                                    Inferno_rush_timer=30000;
                                    spread_flame_timer=2000;
                                }
                            }
                        }
                        else Inferno_rush_timer-=diff;


                            if(spread_flame_timer<=diff && can_knock)
                            {
                                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                                DoCast(89876); // DMG + knock back
                                me->GetMotionMaster()->MoveCharge(me->getVictim()->GetPositionX(),me->getVictim()->GetPositionY(),me->getVictim()->GetPositionZ(),42.0f);
                                me->clearUnitState(UNIT_STAT_CASTING);
                                can_knock=false;
                            }
                            else spread_flame_timer-=diff;


                            if(Creature* pFeludius = me->FindNearestCreature(FELUDIUS_ENTRY, 500, true) )
                                {
                                    if(pFeludius->HealthBelowPct(25))
                                        PHASE=2;
                                }

             DoMeleeAttackIfReady();
        }  // Koniec faze 1


                     if(Tele_debug_timer<=diff && !debuged)
                     {
                        me->NearTeleportTo(-1044.48f,-551.18f,835.15f, 0.98f);
                        debuged=true;
                     }
                     else Tele_debug_timer-=diff;

                            if(PHASE==2 ||( HealthBelowPct(25) && !Hp_dropped))
                            {
                                Tele_debug_timer=500;
                                debuged=false;
                                me->InterruptNonMeleeSpells(true);
                                DoCast(me,87459); // Visual teleport
                                PHASE=3;
                                me->RemoveAllAuras();
                                me->SetReactState(REACT_PASSIVE);
                                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                                me->AttackStop();
                                float x, y, z, o;
                                me->GetHomePosition(x, y, z, o);
                                me->NearTeleportTo(x, y, z, o);
                                me->SummonCreature(TERRASTRA_ENTRY,-1053.943f,-569.11f,835.2f, 6.0f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
                                DoCast(me,83811); // Visual cast fire evocation ( stali v combat pozicii lepsie takto zatial :D )
                                Hp_dropped=true;
                            }

                            if(PHASE==3) //  Zakamuflujem sa a portnem sa na spawn position ak sa spawnla Monstrosity :P
                            {
                                if(me->FindNearestCreature(43689, 300, true) == NULL)
                                {
                                    me->SetVisible(false);
                                    float x, y, z, o;
                                    me->GetHomePosition(x, y, z, o);
                                    me->NearTeleportTo(x, y, z, o);
                                    PHASE=4;
                                }
                            }
        }

    };
};


/************************* WATER BOMB *******************************/
class water_bomb : public CreatureScript
{
public:
    water_bomb() : CreatureScript("water_bomb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new water_bombAI (creature);
    }

    struct water_bombAI : public ScriptedAI
    {
        water_bombAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Bomb_timer;
        bool bombed;

        void DamageDealt(Unit* target, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == SPELL_DIRECT_DAMAGE) // Po zasahu bombou aplikujem Watterlogged
                me->AddAura(SPELL_WATER_LOGGED,target);
        }

        void Reset()
        {
            if(Creature *pTarget = me->FindNearestCreature(43687, 500, true))
                Bomb_timer= 2200 + uint32(me->GetDistance2d(pTarget)/7)*1000; // zhruba tolko trva kym visualne doleti bomba k "water bomb npc"
            else Bomb_timer =3000+ urand(500,3000); // keby nahodou

            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE|UNIT_FLAG_STUNNED);
            bombed=false;
        }

        void JustDied (Unit * killed)
        {
            me->setDeathState(DEAD);
            me->RemoveCorpse();
        }

        void UpdateAI(const uint32 diff)
        {
            if(Bomb_timer<=diff && !bombed)
            {
                DoCast(me,SPELL_WATER_B_DMG);
                bombed=true;
                me->ForcedDespawn(Bomb_timer+2000);
            }
            else Bomb_timer-=diff;
        }
    };
};

class fire_patch : public CreatureScript
{
public:
    fire_patch() : CreatureScript("fire_patch") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new fire_patchAI (creature);
    }

    struct fire_patchAI : public ScriptedAI
    {
        fire_patchAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        bool fired;
        uint32 flame_timer;

        void DamageDealt(Unit* target, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == SPELL_DIRECT_DAMAGE) // Ak dam dmg hracovi
                if(target->HasAura(SPELL_WATER_LOGGED)) // a ma na sebe waterlogged debuff
                    target->RemoveAura(SPELL_WATER_LOGGED); // odstranim ho
        }

        void Reset()
        {
            fired=false;
            flame_timer=3000;
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->ForcedDespawn(35000);
            me->GetMotionMaster()->MoveFall(0);
        }

        void UpdateAI(const uint32 diff)
        {
            if(flame_timer<=diff && !fired)
            {
                DoCast(me,SPELL_INFERNO_RUSH_AOE);
                fired=true;
            }
            else flame_timer-=diff;
        }
    };
};

class boss_Arion : public CreatureScript
{
public:
    boss_Arion() : CreatureScript("boss_Arion") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_ArionAI (creature);
    }

    struct boss_ArionAI : public ScriptedAI
    {
        boss_ArionAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        bool fired;
        uint32 Call_winds;
        uint32 rod_timer;
        uint32 Lightning_blast_timer;
        uint32 Thundershock_timer;
        uint32 Disperse_timer;
        uint32 Chain_timer;
        uint32 PHASE;
        uint32 TeleDebug_timer;
        uint32 walk_timer;
        uint32 walk_timer_Feludisu;
        Unit* pRod_marked_player;
        bool can_chaining,ported,can_tele,Hp_dropped,can_interrupt;

        void Reset()
        {
            PHASE=1;
            me->SetSpeed(MOVE_RUN,1.3f,true);
            rod_timer=15000;
            Call_winds=10000;
            Thundershock_timer=70000;
            Disperse_timer=20000;
            Chain_timer=1000;
            pRod_marked_player=NULL;
            can_chaining=ported=can_tele=Hp_dropped=can_interrupt=false;
            pRod_marked_player=NULL;
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect

            me->SetSpeed(MOVE_RUN, 1.4f, true);
            me->SetInCombatWithZone();
        }

        void KilledUnit(Unit * /*victim*/)
        {
            me->MonsterYell("Merely a whisper in the wind...", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20238, false);
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->MonsterYell("Enough of this foolishness!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(20237, false);
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(int i=0;i<3;i++)
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(can_interrupt)
                    {
                        me->InterruptNonMeleeSpells(true);
                        break;
                    }
                }
        }

        void UpdateAI(const uint32 diff)
        {

            if (!UpdateVictim())
                return;

    if(PHASE==1)
    {
        if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE) && !me->IsNonMeleeSpellCasted(false)) // Po Disperse bezal boss za targetom musim to cekovat takto
            me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);

        if(Lightning_blast_timer<=diff && can_tele) // Hned po docasteni Disperse sa portne na random poziciu a vycasti na tanka Lightning blast
        {

            if(!me->IsNonMeleeSpellCasted(false))
                {
                    me->SetVisible(false);
                    int rand_pos=urand(1,4); // Port na jednu zo styroch nahodnych pozicii
                    me->NearTeleportTo(Tele_pos[rand_pos].GetPositionX(),Tele_pos[rand_pos].GetPositionY(),Tele_pos[rand_pos].GetPositionZ(),Tele_pos[rand_pos].GetOrientation());
                    me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    //me->GetMotionMaster()->MoveJump(Tele_pos[rand_pos].GetPositionX(),Tele_pos[rand_pos].GetPositionY(),Tele_pos[rand_pos].GetPositionZ(),200.0f,5.0f);
                    me->SetVisible(true);
                    me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    can_interrupt=true;
                    DoCast(me->getVictim(),83070);// Hned potom zacastim na aktualneho tanka Lightning blast
                    can_tele=false;
                }
        }
        else Lightning_blast_timer-=diff;


        if(Chain_timer<=diff && can_chaining)
        {
            if(!me->IsNonMeleeSpellCasted(false))
                {
                    if(pRod_marked_player && pRod_marked_player->isAlive())
                    {
                        DoCast(pRod_marked_player,83282);
                        Chain_timer=1000;
                        can_chaining=false;
                    }
                }
        }
        else Chain_timer-=diff;


            if(rod_timer<=diff) // Lightning rod (mark) pre chain lightning
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    if ((pRod_marked_player = SelectTarget(SELECT_TARGET_RANDOM, 0, 200, true)) != NULL)
                    {
                        me->AddAura(83099,pRod_marked_player);
                        rod_timer=40000;
                        Chain_timer=12000; // Do marked targetu zacastim chain lightning
                        can_chaining=true;
                    }
                }
            }
            else rod_timer-=diff;


            if(Call_winds<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    DoCast(83491); // summon Viloent cyclone
                    Call_winds=20000;
                    me->MonsterYell("Wind, hear my call!", LANG_UNIVERSAL, NULL);
                    me->SendPlaySound(20239, false);
                }
            }
            else Call_winds-=diff;

            if(Thundershock_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    me->MonsterTextEmote("The air around you crackles with energy...", 0, true);
                    DoCast(me->getVictim(),83067);
                    Thundershock_timer=70000;
                }
            }
            else Thundershock_timer-=diff;

            if(Disperse_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    DoCast(83087);
                    Disperse_timer=30000;
                    can_tele=true;
                    Lightning_blast_timer=50;
                }
            }
            else Disperse_timer-=diff;

            if(Creature* pTerra = me->FindNearestCreature(TERRASTRA_ENTRY, 500, true) )
            {
                if(pTerra->HealthBelowPct(25))
                    PHASE=2;
            }

            DoMeleeAttackIfReady();
    }
/********************************* DRUHA FAZA *********************************************************/
                            if(PHASE==2 || (HealthBelowPct(25) && !Hp_dropped))
                            {
                                if(Creature* pIgnacious = me->FindNearestCreature(IGNACIOUS_ENTRY, 500, true) )
                                {
                                    pIgnacious->NearTeleportTo(-1029.52f,-561.7f,831.92f,5.52f);
                                    pIgnacious->InterruptNonMeleeSpells(true);
                                    pIgnacious->CastSpell(pIgnacious, 87459, true); // Visual teleport
                                }

                                me->InterruptNonMeleeSpells(true);
                                DoCast(me->getVictim(),82285); // Elemental stasis
                                DoCast(me,87459); // Visual teleport
                                PHASE=3;
                                me->RemoveAllAuras();
                                me->SetReactState(REACT_PASSIVE);
                                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                                me->AttackStop();
                                me->NearTeleportTo(-987.17f,-561.25f,831.91f,3.93f);
                                TeleDebug_timer=200;
                                walk_timer=4000;
                                me->ForcedDespawn(15000);
                                walk_timer_Feludisu=7000;
                                DoCast(me,87459); // Visual teleport
                                Hp_dropped=true;
                            }

                            if(PHASE==3 && TeleDebug_timer<=diff)
                            {
                                me->NearTeleportTo(-987.17f,-561.25f,831.91f,3.93f);
                                PHASE=4;
                            }
                            else TeleDebug_timer-=diff;

                            if(PHASE==4 && walk_timer<=diff)
                            {
                                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                                me->SetSpeed(MOVE_WALK, 1.5f, true);
                                me->MonsterYell("An impressive display...", LANG_UNIVERSAL, NULL);
                                me->SendPlaySound(20240, false);
                                me->GetMotionMaster()->MovePoint(0,-1009.1f,-582.5f,831.91f);
                                PHASE=5;
                            }
                            else walk_timer-=diff;

                            if(walk_timer_Feludisu<=diff && PHASE==5)
                            {
                                if(Creature* pFeludius = me->FindNearestCreature(FELUDIUS_ENTRY, 500, true) )
                                {
                                    pFeludius->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                                    pFeludius->InterruptNonMeleeSpells(true);
                                    pFeludius->SetSpeed(MOVE_WALK, 1.5f, true);
                                    pFeludius->MonsterYell(" But now witness true power...", LANG_UNIVERSAL, NULL);
                                    pFeludius->SendPlaySound(20165, false);
                                    pFeludius->GetMotionMaster()->MovePoint(0,-1009.1f,-582.5f,831.91f);
                                    PHASE=6;
                                }
                            }
                            else walk_timer_Feludisu-=diff;

        }
    };
};


class boss_Terrastra : public CreatureScript
{
public:
    boss_Terrastra() : CreatureScript("boss_Terrastra") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_TerrastraAI (creature);
    }

    struct boss_TerrastraAI : public ScriptedAI
    {
        boss_TerrastraAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Gravity_well_timer;
        uint32 Harden_Skin_timer;
        uint32 Eruption_timer;
        uint32 Quake_timer;
        uint32 TeleDebug_timer;
        uint32 walk_timer;
        uint32 walk_timer_Ignacious;
        uint32 Monstrosity_timer;
        uint32 Speaking_timer;
        uint32 PHASE;
        uint32 countdown_timer;
        uint32 HS_counter; // Harden skin counter
        uint32 Hp_gainer; // HP ktore bude mat monstrosity
        bool Hp_dropped,has_shield,speaked,can_interrupt;

        void Reset()
        {
            Quake_timer=30000;
            Eruption_timer=27000;
            Harden_Skin_timer=25000;
            Gravity_well_timer=13000;
            Speaking_timer=3500;
            HS_counter=0;
            Hp_gainer=0;
            Hp_dropped=has_shield=speaked=can_interrupt=false;
            PHASE=1;
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            me->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect

            me->SetSpeed(MOVE_RUN, 1.4f, true);
            me->SetInCombatWithZone();
        }

        void KilledUnit(Unit * /*victim*/)
        {
            me->MonsterYell("The soil welcomes your bones!", LANG_UNIVERSAL, NULL);
            me->SendPlaySound(21842, false);
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(int i=0;i<3;i++)
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(can_interrupt)
                    {
                        me->InterruptNonMeleeSpells(true);
                        break;
                    }
                }
        }


        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

                if(Speaking_timer<=diff && !speaked)
                {
                    me->MonsterYell("We will handle them!", LANG_UNIVERSAL, NULL);
                    me->SendPlaySound(21843, false);
                    speaked=true;
                }
                else Speaking_timer-=diff;
    if(PHASE==1)
    {
            if(has_shield==true && countdown_timer<=diff) // Kazdych 500 ms cekujem ci bossovi hravi prelomili stit (Harden Skin )
            {
                HS_counter++;
                countdown_timer=500;

                if(HS_counter==3 && !me->HasAura(83718)) // 1 sekundovy cast time Harden skinu ( Po 1.5 sekunde skontrolujem ci sa podaril spell vycastit)
                {
                    has_shield=false;
                    HS_counter=0;
                }

                if(!me->HasAura(83718) && HS_counter<=60 && HS_counter>3 ) // 60 pretoze 30 s duration ale checkujem kazdych 500 ms nie sekundu
                {
                    me->DealDamage(me,500000, NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    has_shield=false;
                    HS_counter=0;
                }


            }
            else countdown_timer-=diff;

            if(Quake_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    DoCast(me,83565); // Quake
                    Quake_timer=60000;
                    me->MonsterYell("The earth will devour you!", LANG_UNIVERSAL, NULL);
                    me->SendPlaySound(21844, false);
                    me->MonsterTextEmote("The ground beneath you rumbles ominously...", 0, true);
                }
            }
            else Quake_timer-=diff;


            if(Harden_Skin_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=true;
                    DoCast(me,83718); // Harden skin
                    countdown_timer=500;// Kazdych
                    has_shield=true;
                    Harden_Skin_timer=40000;
                }
            }
            else Harden_Skin_timer-=diff;

            if(Eruption_timer<=diff)
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    can_interrupt=false;
                    // Summon 5 Eruption spike okolo hraca
                    DoCast(83675); // Only visual Eruption
                    me->SummonCreature(CREATURE_ERUPTION_NPC,me->getVictim()->GetPositionX()-3.4,me->getVictim()->GetPositionY()-2,me->getVictim()->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    me->SummonCreature(CREATURE_ERUPTION_NPC,me->getVictim()->GetPositionX()-2.4,me->getVictim()->GetPositionY()+3.2,me->getVictim()->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    me->SummonCreature(CREATURE_ERUPTION_NPC,me->getVictim()->GetPositionX()+1.1,me->getVictim()->GetPositionY()+3.2,me->getVictim()->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    me->SummonCreature(CREATURE_ERUPTION_NPC,me->getVictim()->GetPositionX()+0.31,me->getVictim()->GetPositionY()-3.8,me->getVictim()->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    me->SummonCreature(CREATURE_ERUPTION_NPC,me->getVictim()->GetPositionX()+2.2,me->getVictim()->GetPositionY()-0.31,me->getVictim()->GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    Eruption_timer=33000;
                }
            }
            else Eruption_timer-=diff;

            if(Gravity_well_timer<=diff) // Summon jednu gravity well na random poziciu
            {
                if(!me->IsNonMeleeSpellCasted(false))
                {
                    int rand = urand(1,6);
                    me->SummonCreature(CREATURE_GRAVITY_WELL,Gravity_pos[rand].GetPositionX(),Gravity_pos[rand].GetPositionY(),Gravity_pos[rand].GetPositionZ(),0.0f,TEMPSUMMON_CORPSE_DESPAWN, 0);
                    Gravity_well_timer=30000;
                }
            }
            else Gravity_well_timer-=diff;

            if(Creature* pArion = me->FindNearestCreature(ARION_ENTRY, 500, true) )
            {
                if(pArion->HealthBelowPct(25))
                    PHASE=2;
            }

            DoMeleeAttackIfReady();
    }
/********************************* DRUHA FAZA *********************************************************/
                            if(PHASE==2 || (HealthBelowPct(25) && !Hp_dropped))
                            {
                                if(Creature* pfel = me->FindNearestCreature(FELUDIUS_ENTRY, 500, true) )
                                {
                                    pfel->InterruptNonMeleeSpells(true);
                                    pfel->NearTeleportTo(-1023.2f,-600.15f,831.91f,0.79f);
                                    pfel->CastSpell(pfel, 87459, true); // Visual teleport
                                }
                                me->InterruptNonMeleeSpells(true);
                                DoCast(me,87459); // Visual teleport
                                PHASE=3;
                                me->RemoveAllAuras();
                                me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
                                me->SetReactState(REACT_PASSIVE);
                                me->AttackStop();
                                me->GetMotionMaster()->MovementExpired();
                                me->NearTeleportTo(-987.68f,-603.96f,831.91f,2.35f);
                                TeleDebug_timer=300;
                                walk_timer=1000;
                                walk_timer_Ignacious=10000;
                                Monstrosity_timer=14000;
                                me->ForcedDespawn(15000);
                                DoCast(me,87459); // Visual teleport
                                Hp_dropped=true;
                            }

                            if(PHASE==3 && TeleDebug_timer<=diff) // musel so mdat tele este raz bugovalo sa to visualne
                            {
                                me->NearTeleportTo(-987.68f,-603.96f,831.91f,2.35f);
                                if(Creature* pfel = me->FindNearestCreature(FELUDIUS_ENTRY, 500, true) )
                                    pfel->NearTeleportTo(-1023.2f,-600.15f,831.91f,0.79f);
                                PHASE=4;
                                walk_timer=6000;
                            }
                            else TeleDebug_timer-=diff;

                            if(PHASE==4 && walk_timer<=diff)
                            {
                                me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                                me->SetSpeed(MOVE_WALK, 1.5f, true);
                                me->MonsterYell("To have made it this far.", LANG_UNIVERSAL, NULL);
                                me->SendPlaySound(21845, false);
                                me->GetMotionMaster()->MovePoint(0,-1009.1f,-582.5f,831.91f);
                                PHASE=5;
                            }
                            else walk_timer-=diff;

                            if(walk_timer_Ignacious<=diff && PHASE==5)
                            {
                                if(Creature* pIgnac = me->FindNearestCreature(IGNACIOUS_ENTRY, 500, true) )
                                {
                                    pIgnac->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                                    pIgnac->InterruptNonMeleeSpells(true);
                                    pIgnac->SetSpeed(MOVE_WALK, 1.5f, true);
                                    pIgnac->MonsterYell("The fury of the elements!", LANG_UNIVERSAL, NULL);
                                    pIgnac->SendPlaySound(20288, false);
                                    pIgnac->GetMotionMaster()->MovePoint(0,-1009.1f,-582.5f,831.91f);
                                    PHASE=6;
                                }
                            }
                            else walk_timer_Ignacious-=diff;

                            if(Monstrosity_timer<=diff && PHASE==6)
                            {
                                PHASE=7;
                                DoCast(me,78771); // Visual Explosion when Monstrosity spawn
                                Creature* Monstrosity=me->SummonCreature(43735,me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(), 6.25f,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,10000);

                                if(Creature* pIgnac = me->FindNearestCreature(IGNACIOUS_ENTRY, 500, true) )
                                    Hp_gainer=Hp_gainer+pIgnac->GetHealth();


                                if(Creature* pfel = me->FindNearestCreature(FELUDIUS_ENTRY, 500, true) )
                                    Hp_gainer=Hp_gainer+pfel->GetHealth();


                                if(Creature* pIgnac = me->FindNearestCreature(ARION_ENTRY, 500, true) )
                                {
                                    Hp_gainer=Hp_gainer+pIgnac->GetHealth();
                                    Hp_gainer=Hp_gainer+me->GetHealth(); // + HP Terrastry
                                }


                                Monstrosity->SetInCombatWithZone();
                                Monstrosity->SetHealth(Hp_gainer);// Monstrosity bude mat tolko HP kolko zostalo elementalom dokopy
                            }
                            else Monstrosity_timer-=diff;
        }
};
};


class Violent_cyclone : public CreatureScript
{
public:
    Violent_cyclone() : CreatureScript("Violent_cyclone") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Violent_cycloneAI (creature);
    }

    struct Violent_cycloneAI : public ScriptedAI
    {
        Violent_cycloneAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Buff_timer;
        bool buffed;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            me->ForcedDespawn(25000);
            Buff_timer=1000;
            buffed=false;
        }

        void DamageDealt(Unit* target, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == SPELL_DIRECT_DAMAGE && target->GetTypeId() == TYPEID_PLAYER)
            { // Po zasahu tornadom hrac dostane buff ( swirling winds)->levitate
                me->AddAura(83500,target);
                if(target->HasAura(83581)) // Ak ma hrac grounded debuff zhodim mu ho
                    target->RemoveAura(83581);
            }
        }


        void UpdateAI(const uint32 diff)
        {
            if(me->FindNearestCreature(43735, 500, true) != NULL)
                me->ForcedDespawn();

            if(Buff_timer<=diff && !buffed)
            {
                me->SetSpeed(MOVE_RUN,0.5f,true);
                DoCast(me,83472); //effekt tornada
                DoCast(me,87870); // knockback
                me->GetMotionMaster()->MoveRandom(60.0f);
                buffed=true;
            }
            else Buff_timer-=diff;
        }
    };
};

class Gravity_well : public CreatureScript
{
public:
    Gravity_well() : CreatureScript("Gravity_well") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Gravity_wellAI (creature);
    }

    struct Gravity_wellAI : public ScriptedAI
    {
        Gravity_wellAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Buff_timer;
        bool buffed;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->ForcedDespawn(35000);
            Buff_timer=1000;
            buffed=false;
        }

        void DamageDealt(Unit* target, uint32& /*damage*/, DamageEffectType typeOfDamage)
        {
            if(typeOfDamage == SPELL_DIRECT_DAMAGE && target->GetTypeId() == TYPEID_PLAYER) // Po zasahu wellom hrac dostane debuff grounded
            {
                if(!target->HasAura(83581)) // Ak sa hrac ktory nema grounded debuff priblizi ku wellu vcucne ho to :D
                target->GetMotionMaster()->MoveJump(me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(),30.0f,2.0f);
                me->AddAura(83581,target); // grounded debuff
                if(target->HasAura(83500)) // Ak ma hrac auru swirling winds zhodim mu ju
                    target->RemoveAurasDueToSpell(83500);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if(me->FindNearestCreature(43735, 500, true) != NULL)
                me->ForcedDespawn();
            if(Buff_timer<=diff && !buffed)
            {
                DoCast(me,83579); //effekt Gravity wellu + aoe dmg
                DoCast(me,83587); // magnetic pull - 50 % movement speed v okoli wellu
                buffed=true;
            }
            else Buff_timer-=diff;
        }
    };
};

class Eruption_spike : public CreatureScript
{
public:
    Eruption_spike() : CreatureScript("Eruption_spike") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new Eruption_spikeAI (creature);
    }

    struct Eruption_spikeAI : public ScriptedAI
    {
        Eruption_spikeAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        uint32 Buff_timer;
        uint32 Spike_dmg_timer;
        bool buffed;
        bool spiked;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->ForcedDespawn(5000);
            Buff_timer=1000;
            Spike_dmg_timer = 3000;
            buffed=spiked=false;
        }

        void UpdateAI(const uint32 diff)
        {
            if(Buff_timer<=diff && !buffed)
            {
                DoCast(me,83662); // pure visual forecast spike
                buffed=true;
            }
            else Buff_timer-=diff;

            if(Spike_dmg_timer<=diff && !spiked)
            {
                DoCast(me,92536); // spike + dmg
                spiked=true;
            }
            else Spike_dmg_timer-=diff;
        }
    };
};


void AddSC_twilight_council()
{
    new mob_felo();
    new mob_ignac();
    new boss_Arion();
    new boss_Terrastra();
    new water_bomb();
    new fire_patch();
    new Violent_cyclone();
    new Gravity_well();
    new Eruption_spike();
    new boss_Elementium_Monstrosity();
    new mob_lava_seed();
    new instability_bunny();
    new mob_liquide_ice();
}
