//door : 401930

#include "ScriptPCH.h"
#include "bastion_of_twilight.h"
#include "Spell.h"

#define DATA_ACHIEVEMENT            1

enum Sounds
{
    SAY_INTRO                   = 22055,
    SAY_DEATH                    = 22056,
    SAY_DEATH2                  = 20189,
    SAY_AGGRO                    = 20186,
    SAY_KILL                    = 20187,
    SAY_KILL2                    = 20188
};

enum Spells
{
    //HALFUS
    SPELL_DRAGON_VENGEANCE      = 87683,
    SPELL_BERSERK               = 26662,
    SPELL_MALEVOLENT_STRAKES    = 39171,
    SPELL_SHADOW_WRAPPED        = 83952,
    SPELL_BIND_WILL             = 83432,
    SPELL_FRENZIED_ASSAULT      = 83693,
    SPELL_FURIOUS_ROAR          = 83710,
    //SPELL_PARALYSIS                = 84030,
    SPELL_SHADOW_NOVA           = 83703,
    SPELL_PETRIFICATION         = 84591,

    // minion's spells
    SPELL_CYCLONE_WINDS         = 84092,
    //SPELL_STONE_TOUCH           = 83603,
    SPELL_STONE_GRID            = 84593,
    SPELL_NETHER_BLINDNESS      = 83611,
    SPELL_TIME_DILATION         = 83601,
    SPELL_ATROPHIC_POISON       = 83609,

    SPELL_FREE_DRAGON           = 83447,
    SPELL_UNRESPONSIVE          = 86003,
    SPELL_WHELP_UNRESPONSIVE    = 86022,

    // proto-behemoth spells
    SPELL_BALL_FAST             = 83720,
    SPELL_BALL_SLOW             = 83733,
    SPELL_SUPERHEATED_BREATH    = 83956,
    SPELL_DANCING_FLAMES        = 84106,
    SPELL_FIREBALL_BARRAGE      = 83706,
    SPELL_SCORCHING_BREATH      = 83707,
    SPELL_FIREBALL              = 86058,

    //Chains
    SPELL_SPIKE_VISUAL            = 83480,
    SPELL_CHAINS                = 83487,
};

enum MiscData
{
    NPC_SPIKE                    = 44765,
};

enum ePhases
{
    PHASE_1   = 1,
    PHASE_2   = 2,
};

Position const WhelpPositions[8] = 
{
    {-348.196f, -724.997f, 895.0f, 0.0f},
    {-350.587f, -722.574f, 895.0f, 0.0f},
    {-347.585f, -717.91f, 895.0f, 0.0f},
    {-346.367f, -722.663f, 895.0f, 0.0f},
    {-344.332f, -718.729f, 895.0f, 0.0f},
    {-340.578f, -720.856f, 895.0f, 0.0f},
    {-342.117f, -724.913f, 895.0f, 0.0f},
    {-344.564f, -727.78f, 895.0f, 0.0f},
};

Position const ChainPositions[4][5] =
{
    { //Storm Rider
        {-326.635f, -722.1f, 888.086f, 0.0f},
        {-318.456f, -720.084f, 888.086f, 0.0f},
        {-314.765f, -729.858f, 889.0f, 0.0f},
        {-321.26f, -734.317f, 889.0f, 0.0f},
        {-329.331f, -731.596f, 889.0f, 0.0f},
    },
    { //Nether Scion
        {-288.605f, -668.08f, 888.093f, 0.0f},
        {-285.811f, -674.824f, 888.087f, 0.0f},
        {-277.256f, -673.467f, 888.086f, 0.0f},
        {-275.28f, -666.644f, 888.086f, 0.0f},
        {-280.962f, -663.772f, 888.09f, 0.0f},
    },
    { //Time Wardner
        {-356.686f, -691.948f, 888.106f, 0.0f},
        {-350.257f, -693.066f, 888.104f, 0.0f},
        {-347.22f, -699.184f, 888.105f, 0.0f},
        {-356.2f, -701.894f, 888.106f, 0.0f},
        {-359.774f, -698.1f, 888.105f, 0.0f},
    },
    { //Slate Dragon
        {-288.68f, -699.766f, 888.086f, 0.0f},
        {-283.365f, -703.803f, 888.086f, 0.0f},
        {-277.611f, -705.6f, 888.086f, 0.0f},
        {-274.244f, -691.175f, 888.087f, 0.0f},
        {-283.795f, -691.285f, 888.086f, 0.0f},
    }
};

class boss_halfus_wyrmbreaker : public CreatureScript
{
public:
    boss_halfus_wyrmbreaker() : CreatureScript("boss_halfus_wyrmbreaker") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_halfus_wyrmbreakerAI (creature);
    }

    struct boss_halfus_wyrmbreakerAI : public ScriptedAI
    {
        boss_halfus_wyrmbreakerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            Intro = 1;
            //SetHealth
            me->setFaction(16);
            if (Is25ManRaid())
            {
                if (IsHeroic())
                {
                    me->SetMaxHealth(184667808);
                }
                else me->SetMaxHealth(111312000);
            }
            else 
            {
                if (IsHeroic())
                {
                    me->SetMaxHealth(51535200);
                }
                else me->SetMaxHealth(31161600);
            }
            me->SetHealth(me->GetMaxHealth());
        }

        InstanceScript* instance;

        std::list<Creature*> DragonList;
        std::list<GameObject*> DoorList;
        
        uint32 ShadowNovaTimer;
        uint32 BerserkTimer;
        uint32 FuriousRoarTimer;
        uint32 FuriousRoarCount;
        uint32 Phase;
        uint32 InterruptTimer;
        uint32 Intro;

        uint32 AchievementTimer;
        uint8 Stack; //Achievement
        uint32 Achievement;
        bool AchievementCheck;

        bool Interruptable;
        bool StormRider;
        bool CycloneWinds;

        void Reset()
        {
            DespawnCreatures(44650);
            DespawnCreatures(44645);
            DespawnCreatures(44652);
            DespawnCreatures(44797);
            AchievementCheck = false;
            Stack = 0;
            Achievement = 0;
            
            StormRider = false;
            
            Phase = PHASE_1;

            if (Aura* Cyclone = me->GetAura(SPELL_CYCLONE_WINDS))
                me->RemoveAura(Cyclone);
            CycloneWinds = false;

            if (instance)
            {
                instance->SetData(DATA_HALFUS, NOT_STARTED);
                RemoveDoors();
                DespawnCreatures(NPC_SPIKE);
                if (GameObject* Door2 = me->SummonGameObject(205223, -419.185f, -684.397f, 894.58f, 0.00908613f, 0, 0, 0, 0, 0))
                    DoorList.push_back(Door2);
                RemoveDragon();
                SummonDragon();
                if (!IsHeroic())
                    {
                        if (instance->GetData(DATA_STORM_RIDER) == 1)
                            {
                                DoCast(me, SPELL_SHADOW_WRAPPED);
                                StormRider = true;
                            }
                        if (instance->GetData(DATA_THE_SLATE_DRAGON) == 1)
                            DoCast(me, SPELL_MALEVOLENT_STRAKES);
                        if (instance->GetData(DATA_NETHER_SCION) == 1)
                            DoCast(me, SPELL_FRENZIED_ASSAULT);
                    }
                else
                    {
                        DoCast(me, SPELL_FRENZIED_ASSAULT);
                        DoCast(me, SPELL_MALEVOLENT_STRAKES);
                        DoCast(me, SPELL_SHADOW_WRAPPED);
                        StormRider = true;
                    };
            }
            
            me->ApplySpellImmune(SPELL_FURIOUS_ROAR, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(SPELL_FURIOUS_ROAR, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
            me->ApplySpellImmune(SPELL_FURIOUS_ROAR, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
        }

        void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    switch (urand(1, 2))
                    {
                        case 1: me->MonsterYell("The burden of the damned falls upon you!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(me, SAY_KILL2);
                            break;
                        case 2: me->MonsterYell("The wyrms will eat well tonight!", LANG_UNIVERSAL, 0);
                                DoPlaySoundToSet(me, SAY_KILL);
                            break;
                    }
            }

        void SummonDragon()
        {
            if (Creature* Protos = me->SummonCreature(44687, -277.34f, -753.7f, 910.922f, 2.0f, TEMPSUMMON_MANUAL_DESPAWN))
                DragonList.push_back(Protos);
            if (Creature* Summoned2 = me->SummonCreature(44650, -321.283f, -727.229f, 888.086f, 2.82752f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Summoned2->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    DragonList.push_back(Summoned2);
                    if (!IsHeroic())
                        if (!instance->GetData(DATA_STORM_RIDER) == 1)
                            {
                                Summoned2->AddAura(SPELL_UNRESPONSIVE, Summoned2);
                                Summoned2->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            }
                };
            if (Creature* Summoned3 = me->SummonCreature(44645, -282.774f, -668.748f, 888.089f, 1.94274f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Summoned3->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    DragonList.push_back(Summoned3);
                    if (!IsHeroic())
                        if (!instance->GetData(DATA_NETHER_SCION) == 1)
                            {
                                Summoned3->AddAura(SPELL_UNRESPONSIVE, Summoned3);
                                Summoned3->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            }
                };
            if (Creature* Summoned4 = me->SummonCreature(44652, -279.755f, -696.075f, 888.087f, 2.76976f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Summoned4->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    DragonList.push_back(Summoned4);
                    if (!IsHeroic())
                        if (!instance->GetData(DATA_THE_SLATE_DRAGON) == 1)
                            {
                                Summoned4->AddAura(SPELL_UNRESPONSIVE, Summoned4);
                                Summoned4->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            }
                };
            if (Creature* Summoned5 = me->SummonCreature(44797, -353.06f, -699.021f, 888.105f, 5.52467f, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Summoned5->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    DragonList.push_back(Summoned5);
                    if (!IsHeroic())
                        if (!instance->GetData(DATA_THE_TIME_WARDEN) == 1)
                            {
                                Summoned5->AddAura(SPELL_UNRESPONSIVE, Summoned5);
                                Summoned5->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            }
                };
        };

        void RemoveDoors()
            {
                if (DoorList.empty())
                    return;

                for (std::list<GameObject*>::const_iterator itr = DoorList.begin(); itr != DoorList.end(); ++itr)
                    if ((*itr)->IsInWorld())
                        (*itr)->RemoveFromWorld();

                  DoorList.clear();
             };

        void RemoveDragon()
            {
                if (DragonList.empty())
                    return;

                for (std::list<Creature*>::const_iterator itr = DragonList.begin(); itr != DragonList.end(); ++itr)
                    if (*itr)
                        if ((*itr)->IsInWorld())
                            (*itr)->DisappearAndDie();

                  DragonList.clear();
             };

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                for (std::list<Creature*>::iterator itr = DragonList.begin(); itr != DragonList.end();)
                    if ((*itr)->GetEntry() == summon->GetEntry())
                        itr = DragonList.erase(itr);
                    else itr++;
            }

        void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 200.0f);

                if (creatures.empty())
                    return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                    (*iter)->DespawnOrUnsummon();
            }

        uint32 GetData(uint32 type)
            {
                switch (type)
                {
                    case DATA_ACHIEVEMENT:
                        return Achievement;
                        break;
                }
                return 0;
            }

        void SpellHit(Unit* target, const SpellEntry* spell)
        {
            if(!spell)
                return;

            for(uint8 i = 0; i < 3; i++) {
                if(spell->Effect[i] == SPELL_EFFECT_INTERRUPT_CAST)
                {
                    if(Interruptable)
                    {
                        me->InterruptNonMeleeSpells(true);
                        Interruptable = false;
                    }
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            me->MonsterYell("Cho'gall will have your heads! ALL OF THEM!", LANG_UNIVERSAL, 0);
            DoPlaySoundToSet(me, SAY_AGGRO);
            if (GameObject* Door1 = me->SummonGameObject(205222, -320.073f, -585.627f, 894.58f, 1.58553f, 0, 0, 0, 0, 86400*IN_MILLISECONDS))
                DoorList.push_back(Door1);
            if (instance)
                instance->SetData(DATA_HALFUS, IN_PROGRESS);
            if (Creature* protos = Unit::GetCreature(*me, instance->GetData64(DATA_PROTO_BEHEMOTH)))
                 DoZoneInCombat(protos);
            ShadowNovaTimer = urand(15*IN_MILLISECONDS, 18*IN_MILLISECONDS);
            BerserkTimer = 360*IN_MILLISECONDS;
            FuriousRoarTimer = 0;
            FuriousRoarCount = 0;
            DoZoneInCombat(me);
            
            Interruptable = false;
        };

        void JustDied(Unit* /*Killer*/)
        {
            DespawnCreatures(44650);
            DespawnCreatures(44645);
            DespawnCreatures(44652);
            DespawnCreatures(44797);
            DespawnCreatures(NPC_SPIKE);
            DespawnCreatures(44641);
            RemoveDoors();
            DoPlaySoundToSet(me, SAY_DEATH2);
            if (Creature* Summoned = me->SummonCreature(51599, -437.9f, -684.63f, 894.6f, 0, TEMPSUMMON_TIMED_DESPAWN, 10000))
                {
                    Summoned->MonsterYell("Flesh and sinew, weak but proud. Dare they part the Master's shroud? They stumble fumble groping blind, Finding fate and chaos intertwined.", LANG_UNIVERSAL, 0);
                    DoPlaySoundToSet(Summoned, SAY_DEATH);
                    Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                };
            instance->SetData(DATA_HALFUS, DONE);
            RemoveDragon();
        };

        void UpdateAI(const uint32 diff)
        {
            if (!CycloneWinds && me->GetAura(SPELL_CYCLONE_WINDS))
                CycloneWinds = true;

            if (instance)
                {
                    Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
                        for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
                            if (Player* player = i->getSource())
                                if (player->IsWithinDistInMap(me, 100) && Intro == 1)
                                    if (Creature* Summoned = me->SummonCreature(51599, -437.9f, -684.63f, 894.6f, 0, TEMPSUMMON_TIMED_DESPAWN, 15000))
                                        {
                                            Intro = 0;
                                            Summoned->MonsterYell("Halfus! Hear me! (The Master calls, the Master wants) Protect our secrets, Halfus. Destroy the intruders. (Murder for His glory. Murder for His hunger.)", LANG_UNIVERSAL, 0);
                                            DoPlaySoundToSet(me, SAY_INTRO);
                                            Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                        };
                };

            if (Aura* vengeance = me->GetAura(SPELL_DRAGON_VENGEANCE))
            {
                if (vengeance->GetStackAmount() == (Stack + 1))
                {
                    if (AchievementCheck)
                        Achievement = 1;
                    Stack = vengeance->GetStackAmount();
                    AchievementCheck = true;
                    AchievementTimer = 10000;
                }
            }

            if (AchievementTimer <= diff && AchievementCheck)
            {
                AchievementCheck = false;
            }
            else AchievementTimer -= diff;

            if (!UpdateVictim())
                return;

            if (StormRider)
            {
                if (ShadowNovaTimer <= diff)
                    {
                        if (me->GetAura(SPELL_PETRIFICATION))
                            {
                                ShadowNovaTimer = urand(15*IN_MILLISECONDS, 17*IN_MILLISECONDS);
                            };
                        Interruptable = true;
                        DoCast(SPELL_SHADOW_NOVA);

                        if (me->GetAura(SPELL_CYCLONE_WINDS))
                            InterruptTimer = 1500;
                        else InterruptTimer = 375;
                    ShadowNovaTimer = urand(15*IN_MILLISECONDS, 17*IN_MILLISECONDS);
                }
                else
                    ShadowNovaTimer -= diff;
            }

            if (InterruptTimer <= diff && Interruptable)
                Interruptable = false;
            else InterruptTimer -= diff;

            if (BerserkTimer <= diff)
            {
                me->AddAura(SPELL_BERSERK, me);
                BerserkTimer = 600*IN_MILLISECONDS;
            }
            else
                BerserkTimer -= diff;

            if (Phase == PHASE_1 && me->HealthBelowPct(50))
                Phase = PHASE_2;

            if (Phase == PHASE_2)
            {
                if (FuriousRoarTimer <= diff)
                    {
                        Interruptable = false;
                        if (me->GetAura(SPELL_PETRIFICATION))
                        {
                            FuriousRoarTimer = urand(2*IN_MILLISECONDS, 5*IN_MILLISECONDS);
                            return;
                        };
                        ShadowNovaTimer = urand(15*IN_MILLISECONDS, 17*IN_MILLISECONDS);

                        if (FuriousRoarCount < 3)
                            {
                                if (Aura* Cyclone = me->GetAura(SPELL_CYCLONE_WINDS))
                                    {
                                        CycloneWinds = true;
                                        me->RemoveAura(Cyclone);
                                    };
                                DoCast(SPELL_FURIOUS_ROAR);
                                if (CycloneWinds)
                                    me->AddAura(SPELL_CYCLONE_WINDS, me);
                                ++FuriousRoarCount;
                                FuriousRoarTimer = 1500;
                            }
                        else
                            {
                                DoCast(SPELL_SHADOW_NOVA);
                                FuriousRoarCount = 0;
                                FuriousRoarTimer = 30*IN_MILLISECONDS;
                            }
                    }
                else
                    FuriousRoarTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_halfus_dragon_prisoner : public CreatureScript
{
public:
    npc_halfus_dragon_prisoner() : CreatureScript("npc_halfus_dragon_prisoner") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        player->CastSpell(creature, SPELL_FREE_DRAGON, false);
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_halfus_dragon_prisonerAI (creature);
    }

    struct npc_halfus_dragon_prisonerAI : public ScriptedAI
    {
        npc_halfus_dragon_prisonerAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setFaction(35);
            instance = creature->GetInstanceScript();
            //SetHealth
            if (Is25ManRaid())
            {
                if (IsHeroic())
                {
                    me->SetMaxHealth(20705640);
                }
                else me->SetMaxHealth(14941360);
            }
            else 
            {
                if (IsHeroic())
                {
                    me->SetMaxHealth(5807580);
                }
                else me->SetMaxHealth(3979640);
            }
            me->SetHealth(me->GetMaxHealth());
        }
        
        InstanceScript* instance;
        std::list<Creature*> SummonList;

        uint32 BuffTimer;
        uint32 FlyTimer;
        uint8 Dragon;
        
        bool Active;

        void Reset()
            {
                me->SetFloatValue(UNIT_FIELD_COMBATREACH, 8.0f);
                Active = false;
                SummonChains();
            };

        void RemoveSummons()
            {
                if (SummonList.empty())
                    return;

                for (std::list<Creature*>::const_iterator itr = SummonList.begin(); itr != SummonList.end(); ++itr)
                    if (*itr)
                        if ((*itr)->IsInWorld())
                            (*itr)->DisappearAndDie();

                 SummonList.clear();
             };

        void JustDied(Unit* /*Killer*/)
        {
            if (instance)
                {
                    RemoveSummons();
                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                    {
                        if (Aura* aura = halfus->GetAura(87683))
                            aura->SetStackAmount(aura->GetStackAmount() + 1);
                        else
                            me->AddAura(87683, halfus);
                    }
                }
        }

        void SummonChains()
        {
            switch(me->GetEntry())
            {
            case NPC_STORM_RIDER:
                Dragon = 0;
                break;
            case NPC_NETHER_SCION:
                Dragon = 1;
                break;
            case NPC_THE_TIME_WARDEN:
                Dragon = 2;
                break;
            case NPC_THE_SLATE_DRAGON:
                Dragon = 3;
                break;
            };

            for(uint8 i=0; i<5 ; i++)
                {
                    if (Creature* Summoned = me->SummonCreature(NPC_SPIKE, ChainPositions[Dragon][i].GetPositionX(), ChainPositions[Dragon][i].GetPositionY(), ChainPositions[Dragon][i].GetPositionZ(), urand(0,6.28), TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        Summoned->CastSpell(me, SPELL_SPIKE_VISUAL, true);
                        Summoned->CastSpell(me, SPELL_CHAINS, true);
                        Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        SummonList.push_back(Summoned);
                    };
                };
        };

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_FREE_DRAGON)
                {   
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))       
                        DoZoneInCombat(halfus, 200.0f);
                    me->SetFlying(true);
                    RemoveSummons();
                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), halfus->GetPositionZ()+7.0f);
                    me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                    BuffTimer = 2000;
                    FlyTimer = 3000;
                    Active = true;
                }

            if (spell->Id == SPELL_BIND_WILL)
                {   
                    me->SetReactState(REACT_AGGRESSIVE);
                    DoZoneInCombat(me, 50000.0f);
                };
        }

        void UpdateAI(const uint32 diff)
        {
            if (Aura* Blindness = me->GetAura(SPELL_NETHER_BLINDNESS))
                me->RemoveAura(Blindness);

            if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                if (me->getVictim() == halfus)
                    halfus->CombatStop(true);

            if (BuffTimer <= diff && Active)
                {
                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                        {
                            me->SetFacingToObject(halfus);
                            if (me->GetEntry() == NPC_STORM_RIDER)
                                {
                                    me->CastSpell(halfus, SPELL_CYCLONE_WINDS, false);
                                };
                            if (me->GetEntry() == NPC_THE_SLATE_DRAGON)
                                {
                                    me->CastSpell(halfus, SPELL_STONE_GRID, false);
                                };
                            if (me->GetEntry() == NPC_THE_TIME_WARDEN)
                                {
                                    if (Creature* protos = Unit::GetCreature(*me, instance->GetData64(DATA_PROTO_BEHEMOTH)))
                                    {
                                        me->CastSpell(protos, SPELL_TIME_DILATION, false);
                                        me->SetFacingToObject(protos);
                                    };
                                };
                            if (me->GetEntry() == NPC_NETHER_SCION)
                                {
                                    me->CastSpell(halfus, SPELL_NETHER_BLINDNESS, false);
                                };
                        };
                    BuffTimer = 50000;
                }
            else BuffTimer -= diff;

        if (Active)
            {
                if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                    {
                    if (halfus->GetAura(SPELL_CYCLONE_WINDS) && me->GetEntry() == NPC_STORM_RIDER)
                        {
                            halfus->CastSpell(me, SPELL_BIND_WILL, true);
                            Active = false;
                        };
                    if (halfus->GetAura(SPELL_STONE_GRID) && me->GetEntry() == NPC_THE_SLATE_DRAGON)
                        {
                            halfus->CastSpell(me, SPELL_BIND_WILL, true);
                            Active = false;
                        };
                    if (Creature* protos = Unit::GetCreature(*me, instance->GetData64(DATA_PROTO_BEHEMOTH)))
                        if (protos->GetAura(SPELL_TIME_DILATION) && me->GetEntry() == NPC_THE_TIME_WARDEN)
                            {
                                halfus->CastSpell(me, SPELL_BIND_WILL, true);
                                Active = false;
                            };
                    if (halfus->GetAura(SPELL_NETHER_BLINDNESS) && me->GetEntry() == NPC_NETHER_SCION)
                        {
                            halfus->CastSpell(me, SPELL_BIND_WILL, true);
                            Active = false;
                        };
                    if (UpdateVictim())
                        me->GetMotionMaster()->MovePoint(0, me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), me->getVictim()->GetPositionZ()+7.0f);
                    };
            };

            if (!UpdateVictim())
                return;

            if ((FlyTimer <= diff || me->GetMotionMaster()->GetCurrentMovementGeneratorType() == 0) && me->GetDistance(me->getVictim()) >= 2.0f)
            {
                me->GetMotionMaster()->MovePoint(0, me->getVictim()->GetPositionX(), me->getVictim()->GetPositionY(), me->getVictim()->GetPositionZ()+7.0f);
                FlyTimer = 500;
            }
            else FlyTimer -= diff;

            if (me->GetDistance(me->getVictim()) < 1.5f)
                me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());

            if (me->isAttackReady() && !me->IsNonMeleeSpellCasted(false))
            {
               if (me->GetDistance(me->getVictim()) < 2.0f)
                {
                    FlyTimer = 500;
                    me->AttackerStateUpdate(me->getVictim());
                    me->resetAttackTimer();
                }
             }
        }
    };
};

class boss_proto_behemoth : public CreatureScript
{
public:
    boss_proto_behemoth() : CreatureScript("boss_proto_behemoth") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_proto_behemothAI (creature);
    }

    struct boss_proto_behemothAI : public ScriptedAI
    {
        boss_proto_behemothAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->setFaction(16);
            me->SetFlying(true);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
            me->SetReactState(REACT_PASSIVE);
        }

        std::list<Creature*> WhelpsList;
        std::list<GameObject*> Cage;

        InstanceScript* instance;

        uint32 ControlTimer;
        uint32 BreathTimer;
        uint32 BarrageTimer;
        uint32 FireballTimer;
        uint32 WhelpDebuff;
        uint32 BallTimer;
        uint32 Position;

        float PosX;
        float PosY;

        bool CageOpen;
        bool Whelp;
        bool Warden;
        bool Free;

        void Reset() 
        {
            PosX = 0.0f;
            PosY = 0.0f;
            Position = 0;
            Free = false;
            DespawnCreatures(44641);
            WhelpsList.clear();
            Cage.clear();
            Whelp = false;
            Warden = false;
            CageOpen = true;
            WhelpDebuff = 0;

            me->RemoveAura(SPELL_ATROPHIC_POISON);
            SummonWhelp();

            if (instance)
                {
                    if (IsHeroic())
                        {
                            Whelp = true;
                            DoCast(me, SPELL_DANCING_FLAMES);
                            Warden = true;
                        }
                    else if (instance->GetData(DATA_ORPHANED_EMERALD_WHELP) == 1)
                            Whelp = true;
                    else Whelp = false;

                    if (!IsHeroic())
                        if (instance->GetData(DATA_THE_TIME_WARDEN) == 1)
                            {
                                DoCast(me, SPELL_DANCING_FLAMES);
                                Warden = true;
                            }
                        else Warden = false;
                    else Warden = true;

                    if (Whelp)
                        {
                            me->CastSpell(me, SPELL_SUPERHEATED_BREATH, false);
                            for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                                if (CageOpen)
                                        {
                                            if (Creature* Summoned = me->SummonCreature(NPC_SPIKE, -345.997f, -721.726f, 888.095f, 0,TEMPSUMMON_MANUAL_DESPAWN))
                                            {
                                                Summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                                GameObject* Obje2 = Summoned->SummonGameObject(182943, -345.997f, -721.726f, 888.095f, 5.7f, 0, 0, 0, 0, 0); //base
                                                Cage.push_back(Obje2);
                                                GameObject* Obje = Summoned->SummonGameObject(182942, -345.997f, -721.726f, 888.00f, 5.7f, 0, 0, 0, 0, 0);//cage;
                                                Cage.push_back(Obje);
                                                CageOpen = false;
                                            };
                                        };
                        } else
                                {
                                    GameObject* Obje2 = me->SummonGameObject(182943, -345.997f, -721.726f, 888.095f, 5.7f, 0, 0, 0, 0, 0);
                                    Cage.push_back(Obje2);
                                    GameObject* Obje = me->SummonGameObject(182942, -345.997f, -721.726f, 888.00f, 5.7f, 0, 0, 0, 0, 0);
                                    Cage.push_back(Obje);
                                    for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                                            (*itr)->CastSpell((*itr), SPELL_WHELP_UNRESPONSIVE,true);
                                };
            }
        }

        void EnterCombat(Unit* /*who*/)
            {
                FireballTimer = 4*IN_MILLISECONDS;
                BarrageTimer = 23*IN_MILLISECONDS;
                BreathTimer = 10*IN_MILLISECONDS;
            };

        void SummonWhelp()
        {    
            for(uint8 i=0; i<8 ; i++)
            if (Creature* Whelps = me->SummonCreature(44641, WhelpPositions[i].GetPositionX(), WhelpPositions[i].GetPositionY(), WhelpPositions[i].GetPositionZ(), urand(0,6), TEMPSUMMON_MANUAL_DESPAWN))
                {
                    Whelps->setFaction(35);
                    WhelpsList.push_back(Whelps);
                    Whelps->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    //SetHealth
                    if (Is25ManRaid())
                    {
                        if (IsHeroic())
                        {
                            Whelps->SetMaxHealth(20705640);
                        }
                        else Whelps->SetMaxHealth(14941360);
                    }
                    else 
                    {
                        if (IsHeroic())
                        {
                            Whelps->SetMaxHealth(5807580);
                        }
                        else Whelps->SetMaxHealth(3979640);
                    }
                    Whelps->SetMaxHealth(Whelps->GetMaxHealth()/8);
                    Whelps->SetHealth(Whelps->GetMaxHealth());
                };
        };

        void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                if (creatures.empty())
                    return;

                for (std::list<Creature*>::iterator iter = creatures.begin(); iter != creatures.end(); ++iter)
                    (*iter)->DespawnOrUnsummon();
            }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                if (summon->GetEntry() == 44641)
                {
                    WhelpsList.remove(summon);
                    ++WhelpDebuff;
                };
            }

        void UpdateAI(const uint32 diff)
        {
            if (WhelpDebuff == 8)
                {
                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                    {
                        if (Aura* aura = halfus->GetAura(87683))
                            aura->SetStackAmount(aura->GetStackAmount() + 1);
                        else
                            me->AddAura(87683, halfus);
                    }
                    WhelpDebuff = 0;
                 };
            if (!Cage.empty())
            for (std::list<GameObject*>::const_iterator itr = Cage.begin(); itr != Cage.end(); ++itr)
                if ((*itr)->IsInWorld())
                    if ((*itr)->GetGoState() == 0 && !CageOpen)
                        for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                            if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                                {
                                    if (PosX == 0.0f || PosX == -338.9981f+6.0f)
                                        PosX = -338.9981f;
                                    if (PosY == 0.0f)
                                        PosY = -711.216675f;
                                    if ((*itr)->isAlive())
                                        (*itr)->GetMotionMaster()->MovePoint(3, PosX, PosY, 888.096802f);
                                    if (Position < 6)
                                    {
                                        PosY = -713.22f;
                                    }
                                    else if (Position < 9)
                                    {
                                        PosY = -715.22f;
                                    };
                                    PosX += 2.0f;
                                    DoZoneInCombat(halfus);
                                    CageOpen = true;
                                    Free = true;
                                    ControlTimer = 2*IN_MILLISECONDS;
                                    ++Position;
                                };

            if (Free)
                {
                    if (WhelpsList.empty())
                        {
                            Free = false;
                            return;
                        };
                    for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                        if (!(*itr))
                        {
                            Free = false;
                            return;
                        };

                    for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                        if (!(*itr)->isAlive() || !(*itr)->IsInWorld())
                        {
                            Free = false;
                            return;
                        };

                    if (!me->GetAura(SPELL_ATROPHIC_POISON) && ControlTimer <= diff)
                        {
                            for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                                {
                                    (*itr)->SetFacingToObject(me);
                                    (*itr)->CastSpell(me, SPELL_ATROPHIC_POISON, false);
                                }
                            ControlTimer = 100*IN_MILLISECONDS;
                        }
                    else
                        ControlTimer -= diff;

                    for (std::list<Creature*>::const_iterator itr = WhelpsList.begin(); itr != WhelpsList.end(); ++itr)
                            {
                                if (me->HasAura(SPELL_ATROPHIC_POISON))
                                {
                                    if (Creature* halfus = Unit::GetCreature(*me, instance->GetData64(DATA_HALFUS)))
                                        if (!(*itr)->GetAura(SPELL_BIND_WILL, halfus->GetGUID()))
                                        {
                                            halfus->CastSpell((*itr), SPELL_BIND_WILL, false);
                                            DoZoneInCombat((*itr));
                                            (*itr)->clearUnitState(UNIT_STAT_EVADE);
                                            Free = false;
                                        };
                                };
                            };
                }

            if (!UpdateVictim())
                return;

            if (Aura* Poison = me->GetAura(SPELL_ATROPHIC_POISON))
            {
                if (Poison->GetStackAmount() < 8)
                    Poison->SetStackAmount(8);
            };

            if (Warden)
                {
                    if (BarrageTimer <= diff)
                        {
                            BreathTimer = 15*IN_MILLISECONDS;
                            FireballTimer = 11*IN_MILLISECONDS;
                            BallTimer = 500;
                            me->CastSpell(me->getVictim(), SPELL_FIREBALL_BARRAGE, false);
                            BarrageTimer = 30*IN_MILLISECONDS;
                        }
                    else
                        BarrageTimer -= diff;
                };
            if (!Whelp || !Warden)
            {
                if (FireballTimer <= diff)
                    {
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 200.0f, true))
                            if (!me->hasUnitState(UNIT_STAT_CASTING))
                                me->CastSpell(target, SPELL_FIREBALL, false);
                        BallTimer = 1499;
                        FireballTimer = urand(2*IN_MILLISECONDS,4*IN_MILLISECONDS);
                    }
                    else
                        FireballTimer -= diff;
            }

            if (BallTimer <= diff && me->hasUnitState(UNIT_STAT_CASTING))
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 10000, true))
                    {
                        if (me->GetAura(SPELL_TIME_DILATION))
                            {
                                me->CastSpell(target, SPELL_BALL_SLOW, true);
                                BallTimer = 500;
                            }
                        else {
                                me->CastSpell(target, SPELL_BALL_FAST, true);
                                BallTimer = 500;
                            };
                    }
                }
            else BallTimer -= diff;

            if (BreathTimer <= diff && Whelp)
                {
                    BallTimer = 30*IN_MILLISECONDS;
                    FireballTimer = 11*IN_MILLISECONDS;
                    BarrageTimer = 15*IN_MILLISECONDS;
                    me->CastSpell(me->getVictim(), SPELL_SCORCHING_BREATH, true);
                    BreathTimer = 30*IN_MILLISECONDS;
                }
             else
                 BreathTimer -= diff;
        }
    };
};

class spell_superhearted_breath : public SpellScriptLoader
{
    public:
        spell_superhearted_breath() : SpellScriptLoader("spell_superhearted_breath") { }

        class spell_superhearted_breath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_superhearted_breath_SpellScript);

            void ChangeDamage()
            {
               if (GetCaster()->GetAura(SPELL_ATROPHIC_POISON))
                    if (int32 damage = GetHitDamage() - 6000)
                        SetHitDamage(damage);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_superhearted_breath_SpellScript::ChangeDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_superhearted_breath_SpellScript;
        }
};

class spell_stone_grid : public SpellScriptLoader
{
public:
    spell_stone_grid() : SpellScriptLoader("spell_stone_grid") { }

    class spell_stone_grid_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_stone_grid_AuraScript);

        void OnPeriodic(AuraEffect const* /*aurEff*/)
        {
            GetTarget()->AddAura(SPELL_PETRIFICATION, GetTarget());
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_stone_grid_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_stone_grid_AuraScript();
    }
};

class achievement_the_only_escape : public AchievementCriteriaScript
{
    public:
        achievement_the_only_escape() : AchievementCriteriaScript("achievement_the_only_escape") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target)
                return false;

            return target->GetAI()->GetData(DATA_ACHIEVEMENT) == 1;
        }
};

void AddSC_boss_halfus_wyrmbreaker()
{
    new boss_halfus_wyrmbreaker();
    new boss_proto_behemoth();
    new npc_halfus_dragon_prisoner();
    new spell_superhearted_breath();
    new spell_stone_grid();
    new achievement_the_only_escape();
}
