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
#include "well_of_eternity.h"

#define MAX_KILL_QUOTES 3
 
namespace Mannoroth
{
    SimpleQuote onAggro = {26478, "Come, Stormrage, and I will show you what happens to those that betray the lord of the Legion!"}; // DONE
    SimpleQuote onSummonDebilitators = {26488, "[Demonic] Amanare maev il azgalada zila ashj ashj zila enkil!"}; // should be in Demonic
    SimpleQuote onSacrificeVarothen = {26483, "Your blood is MINE, elf!"}; // [That's Not Canon!] achievement
    SimpleQuote onBladeHit = {26482, "Rrraaaghhh!"};
    SimpleQuote onDefeated = {26479, "No... no! This victory will not be ripped from my grasp! I will not return to him in failure! I will not be torn from this pitiful world! No... NOOOOOOOO!!"};
    SimpleQuote onNetherTear = { 26481, "Lord Sargeras, I will not fail you! Sweep your molten fist through this world so that it may be reborn in flames and darkness!" };

    SimpleQuote onKill [MAX_KILL_QUOTES] = // DONE
    {
        {26485, "Fall and die before me!"},
        {26486, "Squirm...Scream. Hahahaha!"},
        {26487, "Useless!"}
    };
}
 
namespace Varothen
{
    SimpleQuote onAggro = {26134, "For you, Azshara!"}; // DONE
    SimpleQuote onDeath = {26135, "Lights of lights! I have failed you. I am sorry, my Azshara..."}; // DONE
 
    SimpleQuote onKill [MAX_KILL_QUOTES] = // DONE
    {
        {26138, "None may cross the queen!"},
        {26139, "A deserved death!"},
        {26140, "In Azshara's name!"}
    };
}
 
namespace Illidan
{
    SimpleQuote onBladeThrow = {26095,"The sword has pierced his infernal armor! Strike him down!"};
    SimpleQuote stillConnected = {26099, "He is still connected to the well somehow! Focus your attacks on Mannoroth, we must disrupt his concentration!" };
}
 
namespace Tyrande
{
    SimpleQuote onAggro = {25997, "I will handle the demons. Elune, guide my arrows!"}; // say
    SimpleQuote onOverhelmed = {25998, "Light of Elune, save me!"};
    SimpleQuote onRelieved = {25999, "I will hold them back for now!"};
    SimpleQuote onPortalCollapsing = {26003, "Malfurion, he has done it! The portal is collapsing!"}; // say -> todo get timer
    SimpleQuote onOutOfArrows = { 26000, "Illidan, I am out of arrows! Moon goddess, protect us from the darkness, that we may see your light again another night!" };
}

#define MAX_PHASE3_QUOTES 7

QUOTE_EVENTS phase3[MAX_PHASE3_QUOTES] =
{
    {11000, MANNOROTH_ENTRY, "Yes... yes! I can feel his burning eyes upon me, he is close...so close. And then your world will be unmade, your lives as nothing!", 26484},
    {8000, ENTRY_TYRANDE_PRELUDE, "There are too many of them!", 26004},
    {10000, ENTRY_ILLIDAN_PRELUDE, "You are not the sole wielder of Sargeras' power, Mannoroth! Behold!", 26096},
    {8000, ENTRY_TYRANDE_PRELUDE, "Illidan... you mustn't!", 26001},
    {10000, ENTRY_ILLIDAN_PRELUDE, "I will be the savior of our people! I WILL FULFILL MY DESTINY!", 26098},
    {1000, ENTRY_TYRANDE_PRELUDE, "No! Illidan!", 0 }, // TODO: sound id
    {MAX_TIMER, ENTRY_ILLIDAN_PRELUDE, "[Demonic] Revos ill ok mordanas archim maz naztheros! Archim xi ante maz-re mishun le nagas!", 26097} // demonic
};
 
#define MAX_OUTRO_QUOTES 12
 
QUOTE_EVENTS outro[MAX_OUTRO_QUOTES] =
{
    {4000, ENTRY_ILLIDAN_PRELUDE, "The artifact!", 26059}, // doladit timing
    {10000, ENTRY_NOZDORMU_PRELUDE, "The Dragon Soul is safe once again. Quickly, into the time portal before this world sunders!", 0}, // nozdormu entry, doladit timing
    {2000, ENTRY_TYRANDE_PRELUDE, "Malfurion...", 25989},
    {5000, ENTRY_MALFURION_PRELUDE, "Hush, Tyrande. Where is Illidan?", 26490},
    {10000, ENTRY_TYRANDE_PRELUDE, "By the very edge...", 25990},
    {6000, ENTRY_ILLIDAN_PRELUDE, "Brother, a timely arrival...", 26060},
    {5000, ENTRY_MALFURION_PRELUDE, "Illidan! The well is out of control!", 26491},
    {30000, ENTRY_ILLIDAN_PRELUDE, "Aye. It's been twisted and turned by too many spells. The fuss we - especially you - made with the portal was too much! The same spell that sent the Burning Legion back to their foul realm now works on the well! It's devouring itself and taking its surroundings with it! Fascinating, isn't it?", 26061},
    {11000, ENTRY_MALFURION_PRELUDE, "Not if we're caught up in it! Why weren't you running! What have you been doing with your hand in the Well?", 26492},
    {11000, ENTRY_ILLIDAN_PRELUDE, "If you've a way out of here, we should probably use it! I've tried casting myself and Tyrande out of here, but the well is too much in flux!", 26062},
    {7000, ENTRY_MALFURION_PRELUDE, "This way!", 26493},
    {MAX_TIMER, ENTRY_TYRANDE_PRELUDE, "I do not know who you are, but I thank you. Without your aid, our world would be... I do not wish to think about it. Moon goddess light your path.", 25991}
};

#define CAST_WOE_INSTANCE(i)     (dynamic_cast<instance_well_of_eternity::instance_well_of_eternity_InstanceMapScript*>(i))

static void PlaySimpleQuote (Creature * source, SimpleQuote quote, bool yell = true)
{
    source->PlayDirectSound(quote.soundID);

    if (yell)
        source->MonsterYell(quote.text, LANG_UNIVERSAL,0,200.0f);
    else
        source->MonsterSay(quote.text, LANG_UNIVERSAL,0,200.0f);
}

static void PlayQuoteEvent(Creature * source, QUOTE_EVENTS qevent, bool yell = true)
{
    source->PlayDirectSound(qevent.soundID);

    if (yell)
        source->MonsterYell(qevent.yellQuote, LANG_UNIVERSAL, 0, 200.0f);
    else
        source->MonsterSay(qevent.yellQuote, LANG_UNIVERSAL, 0, 200.0f);
}

enum entries
{
    ENTRY_VARTOHEN_MAGIC_BLADE = 55837,
    ENTRY_PORTAL_TO_TWISTING_NETHER = 56087,
    ENTRY_MANNOROTH_VEHICLE_RIDER = 55420, // definitely not correct one, but meh ...
    ENTRY_CHROMIE_MANNOROTH = 57913
};

enum spells
{
    SPELL_PORTAL_TO_TWISTING_NETHER = 102920, // freaking long cast time -> just for anim :)
    SPELL_NETHER_TEAR               = 105041, // TARGET_UNIT_NEARBY_ENTRY -> dummy aura missile

    SPELL_FIRESTORM_CHANNEL         = 103888, // summon 55502 triggers ...
    SPELL_FEL_FLAMES_AOE            = 103892,
    SPELL_FELBLADE                  = 103966,
    SPELL_FEL_DRAIN                 = 104961, // instakill to TARGET_UNIT_NEARBY_ENTRY + self heal to 100%
    SPELL_DEBILITATING_FLAY         = 104678, // dummy aura TARGET_UNIT_NEARBY_ENTRY

    SPELL_MAGISTRIKE_CHAIN_DAMAGE   = 103669, // varothens

    SPELL_EMBEDDED_BLADE_VISUAL     = 104823, // in mannoroths chest
    SPELL_EMBEDDED_BLADE_AURA       = 104820, // with scream anim
    SPELL_EMBEDDED_BLADE_AURA2      = 109542, // same spell without anim kit
    SPELL_FEL_FIRE_NOVA_AOE         = 105093,

    SPELL_INFERNO                   = 105141, // apply aura which should periodically trigger missile (spell below)
    SPELL_INFERNO_PERIODIC          = 105145, // triggers unknown spell ...

    SPELL_NETHER_PORTAL_CAST        = 104625, // apply aura, aoe, trigger missile
    SPELL_NETHER_PORTAL_AURA        = 104648,

    SPELL_HAND_OF_ELUNE             = 105072, // infinite channel like spell, periodically trigerring 105073 as AoE (3 max affected targets ...)
    SPELL_HAND_OF_ELUNE_DUMMY_AURA_BLESSING = 109546, // moon above head
    SPELL_MAGISTRIKE_ARC            = 105524 , // trigering 105524=2 + 105523
    SPELL_MAGISTRIKE_PROC_DAMAGE    = 105523, // 1000000 damage to nearby entry (should be self)

    SPELL_GIFT_OF_SARGERAS          = 104998, // 30 s cast ?
    SPELL_GIFT_OF_SARGERAS_INSTANT  = 105009, // but cannot cast ?

    SPELL_ELUNES_WRATH              = 103919, // moonfire like spell

    SPEL_DEBILITATOR_SPAWN_COSMETIC = 104672,

    SPELL_DEMON_PORTAL_PULL_VISUAL_PERIODIC     = 105531, // infinite visual (SPELL_AURA_PERIODIC_DUMMY)
    SPELL_PORTAL_PULL_INIT                      = 105339, // when first hit by visual above ?
    SPELL_PORTAL_PULL_INFINITE_AURA             = 105335, // infinte aura
    SPELL_DEMON_STUN                            = 105430, // when pulling demons

    SPELL_MANNOROTH_FINAL_ANIM_KIT              = 105422,
};

#define ANNIHILATOR_WEAPON_ID 31062
#define MINOR_CACHE_OF_ASPECTS 209541

const Position cachePos = {3356.0f,-5748.0f,15.22f,2.6f};
const Position greenPortalPos = {3344.0f, -5702.0f, 13.4f,4.1f};
const Position debilitatorPos1 = {3294.0f, -5689.0f, 14.7f,5.9f};
const Position debilitatorPos2 = {3322.0f, -5701.0f, 15.8f,2.7f};


const Position demonsPortalPos = { 3382.0f, -5704.0f, 12.0f, 3.8f };

#define DEMON_PORTAL_SUMMONER 55839 // maybe not correct id

namespace PortalDemons
{
    enum portalDemons
    {
        FELHOUND = 56001,
        FELGUARD = 56002,
        DOOMGUARD_DEVASTATOR = 55739,
        INFERNAL = 56036
    };
}

enum bladeSpells
{
    SPELL_VAROTHEN_BLADE_VISUAL_ANIM = 104819,
    SPELL_VAROTHE_BLADE_SUMMON = 104816, // TARGET_DEST_DEST (87)
    SPELL_VAROTHEN_BLADE_MISSILE = 104818,
    SPELL_VAROTHEN_BLADE_SCRIPTED = 104817
};

enum mannorothEncounterActions
{
    ACTION_TYRANDE_DEBILITATOR_DIED = 1111,
    ACTION_START_SUMMON_DEVASTATORS,
    ACTION_STOP_SUMMON_DEVASTATORS
};

class boss_mannoroth_woe : public CreatureScript
{
public:
    boss_mannoroth_woe() : CreatureScript("boss_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_mannoroth_woeAI(creature);
    }

    struct boss_mannoroth_woeAI : public ScriptedAI
    {
        boss_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature), summons(creature)
        {
            pInstance = me->GetInstanceScript();
            me->CastSpell(me, SPELL_PORTAL_TO_TWISTING_NETHER, false);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 30.0f);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 10.0f);

            me->SummonCreature(ENTRY_ABYSSAL_DOOMBRINGER,3274.0f,-5703.0f,16.4f,6.08f);
        }

        SummonList summons;
        InstanceScript * pInstance;
        uint32 wipeCheckTimer;
        uint32 felBladeTimer;
        uint32 felFireStormTimer;
        uint32 fireNovaTimer;
        uint32 achievTimer;
        uint32 debilitatorSpawnTimer;
        uint32 jumpTimer;
        uint32 connectedQuoteTimer;

        uint64 m_focusGUID;

        uint32 phase3QuoteTimer;
        uint32 phase3QuoteCounter;

        uint32 magistrikeCDTimer;

        bool infernoCasted;
        bool canonAchievAllowed;
        bool reached75pct;

        bool portalCollapsed;

        bool canProcMagistirke;

        void Reset() override
        {
            m_focusGUID = 0;

            reached75pct = false;
            canonAchievAllowed = false;
            canProcMagistirke = true;

            magistrikeCDTimer = 5000;

            achievTimer = 1000;

            wipeCheckTimer = 5000;
            felBladeTimer = 5000;
            felFireStormTimer = 15000;
            fireNovaTimer = 5000;
            phase3QuoteTimer = MAX_TIMER;
            connectedQuoteTimer = MAX_TIMER;
            debilitatorSpawnTimer = 65000;
            jumpTimer = MAX_TIMER;
            phase3QuoteCounter = 0;
            infernoCasted = false;
            summons.DespawnAll();
            BoardMannorothPassengers();

            portalCollapsed = false;

            if (pInstance)
            {
                pInstance->SetData(DATA_MANNOROTH, NOT_STARTED);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            ScriptedAI::Reset();
        }

        void AttackStart(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER || victim->GetOwnerGUID() != 0)
                return;

            ScriptedAI::AttackStart(victim);
        }

        void BoardMannorothPassengers()
        {
            if (Vehicle * veh = me->GetVehicleKit())
            {
                uint8 seats = veh->m_Seats.size();

                for (uint8 i = 0; i < seats; i++)
                {
                    if (Creature * vehiclePassenger = me->SummonCreature(ENTRY_MANNOROTH_VEHICLE_RIDER, 0, 0, 0))
                    {
                        vehiclePassenger->EnterVehicle(veh,i);
                        vehiclePassenger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    }
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (!pInstance)
                return;

            // When jumped to portal
            if (type == EFFECT_MOTION_TYPE && id == 0)
            {
                if (Creature * pTwistingNetherPortal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_PORTAL_TO_TWISTING_NETHER)))
                    pTwistingNetherPortal->RemoveAura(SPELL_DEMON_PORTAL_PULL_VISUAL_PERIODIC);

                me->SetVisible(false);
                me->Kill(me);
            }
        }

        void SpellHit(Unit * caster, const SpellEntry * spell) override
        {
            if (spell->Id == SPELL_VAROTHEN_BLADE_SCRIPTED)
            {
                PlaySimpleQuote(me, Mannoroth::onBladeHit);
                if (Creature* pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN_PRELUDE,250.0f,true))
                    PlaySimpleQuote(pIllidan, Illidan::onBladeThrow);

                connectedQuoteTimer = 17000;

                me->CastSpell(me, SPELL_EMBEDDED_BLADE_VISUAL, false);
                me->CastSpell(me, SPELL_EMBEDDED_BLADE_AURA, false);
            }
        }

        void JustSummoned(Creature *pSummoned) override
        {
            summons.Summon(pSummoned);
        }

        void SummonedCreatureDespawn(Creature *pSummoned) override
        {
            summons.Despawn(pSummoned);
        }

        void EnterCombat(Unit * who) override
        {
            if (pInstance)
            {
                if (Creature * pTwistingNetherPortal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_PORTAL_TO_TWISTING_NETHER)))
                    pTwistingNetherPortal->AI()->DoAction(ACTION_START_SUMMON_DEVASTATORS);

                pInstance->SetData(DATA_MANNOROTH, IN_PROGRESS);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            }

            me->InterruptNonMeleeSpells(false);
            me->SetInCombatWithZone();
            PlaySimpleQuote(me, Mannoroth::onAggro);
            ScriptedAI::EnterCombat(who);
        }

        void EnterEvadeMode() override
        {
            if (pInstance)
            {
                if (Creature * pTwistingNetherPortal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_PORTAL_TO_TWISTING_NETHER)))
                    pTwistingNetherPortal->AI()->DoAction(ACTION_STOP_SUMMON_DEVASTATORS);

                pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GIFT_OF_SARGERAS_INSTANT);

                pInstance->SetData(DATA_MANNOROTH, NOT_STARTED);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            if (Creature * pVarothen = ObjectAccessor::GetCreature(*me,pInstance->GetData64(VAROTHEN_ENTRY)))
            {
                if (pVarothen->isDead())
                {
                    pVarothen->setDeathState(JUST_ALIVED);
                    pVarothen->SetFullHealth();
                    pVarothen->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
                    pVarothen->GetMotionMaster()->MoveTargetedHome();
                }
                else
                {
                    pVarothen->AI()->EnterEvadeMode(); // TODO: Why we must force it, something to do with overrided AttackStart + EnterCombat ?
                }
            }
            ScriptedAI::EnterEvadeMode();

            me->CastSpell(me, SPELL_PORTAL_TO_TWISTING_NETHER, false);
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            summons.DespawnAll();
        }

        void JustDied(Unit * killer) override
        {
            summons.DespawnAll();

            if (pInstance)
            {
                pInstance->SetData(DATA_MANNOROTH, DONE);
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (canonAchievAllowed)
                    pInstance->DoCompleteAchievement(6070); // That's Not Canon!
            }

            ScriptedAI::JustDied(killer);
        }

        void DoAction(const int32 action) override
        {
            if (action == 0) // called from Unit::HandleProcTriggerSpell
            {
                canProcMagistirke = false;
                magistrikeCDTimer = 2500;
            }
        }

        uint32 GetData(uint32 type) override
        {
            return canProcMagistirke == true ? 1 : 0;
        }

        void KilledUnit(Unit * victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            PlaySimpleQuote(me, Mannoroth::onKill[urand(0, MAX_KILL_QUOTES - 1)]);
        }

        void ResetEncounter()
        {
            me->AI()->EnterEvadeMode();

            if (Creature * pIllidanPrelude = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_ILLIDAN_PRELUDE)))
                pIllidanPrelude->AI()->DoAction(ACTION_MANNOROTH_ENCOUNTER_FAILED);

            if (Creature * pTyrandePrelude = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_TYRANDE_PRELUDE)))
                pTyrandePrelude->AI()->DoAction(ACTION_MANNOROTH_ENCOUNTER_FAILED);
        }


        void DamageTaken(Unit* done_by, uint32 &damage) override
        {
            if (portalCollapsed == false && me->HealthBelowPct(25))
            {
                portalCollapsed = true;
                OnPortalCollapse();
                return; // just for end of encounter testing, first check 25 % hp condition
            }

            if (infernoCasted == false && me->HealthBelowPctDamaged(51, damage))
            {
                infernoCasted = true;
                me->CastSpell(me, SPELL_INFERNO, false);

                if (Creature * pTyrande = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_TYRANDE_PRELUDE)))
                {
                    pTyrande->AI()->DoAction(ACTION_TYRANDE_PREPARE_WRATH_OF_ELUNE);
                }

                phase3QuoteTimer = 100;
            }

            if (done_by == me) // Can kill self
                return;

            if (portalCollapsed)
                damage = 0;
        }

        void OnPortalCollapse() // Called when HP are at 25% and Mannoroth portal will be pulling all demons to portal
        {
            if (!pInstance)
                return;

            me->SetReactState(REACT_PASSIVE);
            me->InterruptNonMeleeSpells(false);

            PlaySimpleQuote(me, Mannoroth::onDefeated);

            jumpTimer = 30000;

            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);

            Position pos;
            me->GetNearPosition(pos, 10.0f, 1.5f);

            // Le facing hack
            if (Creature * focusTarget = me->SummonCreature(55004, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 0.0f,TEMPSUMMON_TIMED_DESPAWN,30000))
            {
                focusTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
                m_focusGUID = focusTarget->GetGUID();
                me->AddThreat(focusTarget, 9999999.0f);
                me->SetFacingToObject(focusTarget);
                me->SetUInt64Value(UNIT_FIELD_TARGET, focusTarget->GetGUID());
            }

            if (Creature * pIllidan = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_ILLIDAN_PRELUDE)))
            {
                // Summon chromie for quest
                pIllidan->SummonCreature(ENTRY_CHROMIE_MANNOROTH, 3358.52f, -5796.51f, 18.82f, 2.21547f);
                pIllidan->AI()->DoAction(ACTION_MANNOROTH_FIGHT_ENDED);
            }

            if (Creature * pTyrande = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_TYRANDE_PRELUDE)))
            {
                pTyrande->AI()->DoAction(ACTION_MANNOROTH_FIGHT_ENDED);
            }

            me->RemoveAllAuras();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            me->CastSpell(me, SPELL_MANNOROTH_FINAL_ANIM_KIT, true);

            if (Creature * pNetherPortal = me->FindNearestCreature(DEMON_PORTAL_SUMMONER,250.0f,true))
                pNetherPortal->ForcedDespawn();

            if (Creature * pTwistingNetherPortal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_PORTAL_TO_TWISTING_NETHER)))
            {
                pTwistingNetherPortal->AI()->DoAction(ACTION_STOP_SUMMON_DEVASTATORS);
                pTwistingNetherPortal->CastSpell(pTwistingNetherPortal, SPELL_DEMON_PORTAL_PULL_VISUAL_PERIODIC,true);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim() || pInstance == nullptr)
                return;

            if (magistrikeCDTimer <= diff)
            {
                canProcMagistirke = true;
                magistrikeCDTimer = MAX_TIMER;
            }
            else magistrikeCDTimer -= diff;

            if (jumpTimer <= diff)
            {
                if (Creature * pTwistingNetherPortal = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_PORTAL_TO_TWISTING_NETHER)))
                {
                    me->RemoveFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MovementExpired(false);

                    // Jump to portal
                    me->GetMotionMaster()->MoveJump(pTwistingNetherPortal->GetPositionX(),
                                                    pTwistingNetherPortal->GetPositionY(),
                                                    pTwistingNetherPortal->GetPositionZ(),
                                                    50.0f,50.0f,0);
                    // Summon chest
                    me->SummonGameObject(MINOR_CACHE_OF_ASPECTS,    cachePos.GetPositionX(),
                                                                    cachePos.GetPositionY(),
                                                                    cachePos.GetPositionZ(),
                                                                    cachePos.GetOrientation(),
                                                                    0,0,0,0,0);

                    // Clear gift of sargeras
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GIFT_OF_SARGERAS_INSTANT);
                }

                if (Map * map = me->GetMap())
                {
                    for (Map::PlayerList::const_iterator i = map->GetPlayers().begin(); i != map->GetPlayers().end(); ++i)
                    {
                        Player * player = i->getSource();
                        if (player && player->GetQuestStatus(QUEST_THE_PATH_TO_THE_DRAGON_SOUL) == QUEST_STATUS_INCOMPLETE)
                        {
                            player->KilledMonsterCredit(me->GetEntry(), 0);
                        }
                    }
                }

                jumpTimer = MAX_TIMER;
            }
            else jumpTimer -= diff;

            if (portalCollapsed == true)
            {
                if (ObjectAccessor::GetCreature(*me, m_focusGUID))
                    me->SetUInt64Value(UNIT_FIELD_TARGET,m_focusGUID);
                return;
            }

            if (debilitatorSpawnTimer <= diff)
            {
                PlaySimpleQuote(me, Mannoroth::onSummonDebilitators);
                me->SummonCreature(ENTRY_DDREADLORD_DEBILITATOR, debilitatorPos1);
                me->SummonCreature(ENTRY_DDREADLORD_DEBILITATOR, debilitatorPos2);

                me->CastSpell(me, SPELL_NETHER_PORTAL_CAST, false);
                debilitatorSpawnTimer = MAX_TIMER;
            }
            else debilitatorSpawnTimer -= diff;

            if (reached75pct == false && me->HealthBelowPct(76))
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    PlaySimpleQuote(me, Mannoroth::onNetherTear);
                    reached75pct = true;
                    if (Creature * pNetherPortal = me->SummonCreature(DEMON_PORTAL_SUMMONER, demonsPortalPos))
                        me->CastSpell(pNetherPortal, SPELL_NETHER_TEAR, false);
                }
            }

            if (fireNovaTimer <= diff)
            {
                if (me->HasAura(SPELL_EMBEDDED_BLADE_VISUAL))
                    me->CastSpell(me, SPELL_FEL_FIRE_NOVA_AOE, true);
                fireNovaTimer = 6000;
            }
            else fireNovaTimer -= diff;

            if (connectedQuoteTimer <= diff)
            {
                if (Creature * pIllidanPrelude = ObjectAccessor::GetCreature(*me, pInstance->GetData64(ENTRY_ILLIDAN_PRELUDE)))
                    PlaySimpleQuote(pIllidanPrelude,Illidan::stillConnected);

                connectedQuoteTimer = MAX_TIMER;
            }
            else connectedQuoteTimer -= diff;

            if (phase3QuoteTimer <= diff)
            {
                if (phase3QuoteCounter < MAX_PHASE3_QUOTES)
                {
                    uint32 entry = phase3[phase3QuoteCounter].entry;

                    Creature * talker = (entry == MANNOROTH_ENTRY) ? me : nullptr;

                    if (talker == nullptr)
                        talker = me->FindNearestCreature(entry,250.0f,true);

                    if (talker)
                    {
                        switch (phase3QuoteCounter)
                        {
                            case 0:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], true);
                                break;
                            case 1:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], true);
                                break;
                            case 2:
                                talker->CastSpell(talker, SPELL_GIFT_OF_SARGERAS, false); // 30 s cast time
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], true);
                                break;
                            case 3:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], false);
                                break;
                            case 4:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], true);
                                break;
                            case 5:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], false);
                                break;
                            case 6:
                                PlayQuoteEvent(talker, phase3[phase3QuoteCounter], false);
                                break;
                            default:
                                break;
                        }
                    }

                    phase3QuoteTimer = phase3[phase3QuoteCounter++].nextEventTime;
                }
            }
            else phase3QuoteTimer -= diff;

            if (canonAchievAllowed == false && achievTimer < diff)
            {
                if (Creature* pVarothen = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_CAPTAIN_VAROTHEN)))
                {
                    if (me->HealthBelowPct(90) && pVarothen->IsAlive() && !me->IsNonMeleeSpellCasted(false))
                    {
                        PlaySimpleQuote(me, Mannoroth::onSacrificeVarothen);
                        me->CastSpell(pVarothen,SPELL_FEL_DRAIN,false);
                        canonAchievAllowed = true;
                    }
                }
                achievTimer = 1000;
            }
            else achievTimer -= diff;

            if (wipeCheckTimer <= diff)
            {
                if (CAST_WOE_INSTANCE(pInstance)->PlayersWipedOnMannoroth())
                {
                    ResetEncounter();
                    wipeCheckTimer = 5000;
                    return;
                }
                wipeCheckTimer = 5000;
            }
            else wipeCheckTimer -= diff;

            if (felBladeTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me, SPELL_FELBLADE, false);
                    felBladeTimer = 15000; // correct ? +5s after expire
                }
            }
            else felBladeTimer -= diff;

            if (felFireStormTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me, SPELL_FIRESTORM_CHANNEL, false);
                    felFireStormTimer = 30000;
                }
            }
            else felFireStormTimer -= diff;

            if (!me->HasAura(SPELL_MANNOROTH_FINAL_ANIM_KIT))
                DoMeleeAttackIfReady();
        }
    };
};

class boss_captain_varothen_woe : public CreatureScript
{
public:
    boss_captain_varothen_woe() : CreatureScript("boss_captain_varothen_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_captain_varothen_woeAI(creature);
    }

    struct boss_captain_varothen_woeAI : public ScriptedAI
    {
        boss_captain_varothen_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 magistrikeTimer;
        InstanceScript * pInstance;

        void Reset() override
        {
            magistrikeTimer = urand(5000, 8000);
            if (pInstance)
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void EnterEvadeMode() override
        {
            if (pInstance)
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

            ScriptedAI::EnterEvadeMode();
        }

        void AttackStart(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetOwnerGUID() == 0)
                return;

            ScriptedAI::AttackStart(victim);
        }

        void EnterCombat(Unit * victim) override
        {
            if (victim->GetTypeId() == TYPEID_UNIT && victim->GetOwnerGUID() == 0)
                return;

            if (pInstance)
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

            Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true);
            Creature * pIllidan = me->FindNearestCreature(ENTRY_ILLIDAN_PRELUDE, 250.0f, true);
            Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 250.0f, true);

            if (pMannoroth && pIllidan)
            {
                pMannoroth->SetReactState(REACT_AGGRESSIVE);
                pMannoroth->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                pMannoroth->SetInCombatWithZone();
                pIllidan->GetMotionMaster()->MoveChase(pMannoroth);
                pMannoroth->AddThreat(pIllidan, 900000.0f); // is this safe and correct ? check factions ...
                pIllidan->AddThreat(pMannoroth, 900000.0f); // dont attack varothen or somone else
            }

            if (pTyrande)
            {
                pTyrande->AI()->DoAction(ACTION_TYRANDE_START_COMBAT_AFTER_WIPE);
            }

            if (pIllidan)
            {
                pIllidan->AI()->DoAction(ACTION_ILLIDAN_START_COMBAT_AFTER_WIPE);
            }

            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            me->SetInCombatWithZone();
            PlaySimpleQuote(me, Varothen::onAggro);
            ScriptedAI::EnterCombat(victim);
        }

        void KilledUnit(Unit * victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            PlaySimpleQuote(me, Varothen::onKill[urand(0, MAX_KILL_QUOTES - 1)]);
        }

        void JustDied(Unit*) override
        {
            PlaySimpleQuote(me, Varothen::onDeath, false);

            Position pos;
            me->GetNearPosition(pos, 8.0f, me->GetOrientation());
            me->CastSpell(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),SPELL_VAROTHE_BLADE_SUMMON,true);

            if (pInstance)
            {
                pInstance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                if (Map * map = me->GetMap())
                {
                    for (Map::PlayerList::const_iterator i = map->GetPlayers().begin(); i != map->GetPlayers().end(); ++i)
                    {
                        Player * player = i->getSource();
                        if (player && player->GetQuestStatus(QUEST_DOCUMENTING_THE_TIMEWAYS) == QUEST_STATUS_INCOMPLETE)
                        {
                            player->CastSpell(me, SPELL_ARCHIVAL_VAROTHEN_CHANNEL, true);
                            me->CastSpell(player, SPELL_ARCHIVAL_VAROTHEN_CREDIT, true);
                        }
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (magistrikeTimer <= diff)
            {
                me->CastSpell(me->GetVictim(), SPELL_MAGISTRIKE_CHAIN_DAMAGE, true);
                magistrikeTimer = urand(5000, 8000);
            }
            else magistrikeTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_varothen_magic_blade : public CreatureScript
{
public:
    npc_varothen_magic_blade() : CreatureScript("npc_varothen_magic_blade") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_varothen_magic_bladeAI(creature);
    }

    struct npc_varothen_magic_bladeAI : public ScriptedAI
    {
        npc_varothen_magic_bladeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }
        bool clicked = false;

        void EnterEvadeMode() override { }
        void EnterCombat(Unit* /*enemy*/) override {}
        void DamageTaken(Unit* /*who*/, uint32 &damage) override { damage = 0; }
        void Reset() override
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
            me->CastSpell(me, SPELL_VAROTHEN_BLADE_VISUAL_ANIM, true);
        }

        void UpdateAI(const uint32 diff) override
        {
        }

        void DoAction(const int32 action) override
        {
            if (action == EVENT_SPELLCLICK)
            {
                if (!clicked)
                {
                    clicked = true;

                    if (InstanceScript * pInstance = me->GetInstanceScript())
                    if (Creature* pMannoroth = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_MANNOROTH)))
                    {
                        me->CastSpell(pMannoroth, SPELL_VAROTHEN_BLADE_MISSILE, true);
                        me->CastSpell(pMannoroth, SPELL_VAROTHEN_BLADE_SCRIPTED, true);
                    }
                    me->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
                    me->RemoveAura(SPELL_VAROTHEN_BLADE_VISUAL_ANIM);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->ForcedDespawn(10000);
                }
            }
        }
    };
};

enum tyrandeSpells
{
    SPELL_LUNAR_SHOT_AOE = 104688,
    SPELL_BLESSING_OF_ELUNE = 103917, // fix aoe aura application to players
};

enum tyrandeWaypoints
{
    TYRANDE_PORTAL_CASTNG_WP = 5555,
};

class npc_tyrande_mannoroth_woe : public CreatureScript
{
public:
    npc_tyrande_mannoroth_woe() : CreatureScript("npc_tyrande_mannoroth_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_tyrande_mannoroth_woeAI(creature);
    }

    struct npc_tyrande_mannoroth_woeAI : public ScriptedAI
    {
        npc_tyrande_mannoroth_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            lunarShotTimer = MAX_TIMER;
            moveToCombatPosTimer = MAX_TIMER;
            me->SetReactState(REACT_PASSIVE);
            debilitatorDied = 0;
            debilitated = false;
            wrathOfEluneTimer = MAX_TIMER;
        }

        uint32 lunarShotTimer;
        uint32 moveToCombatPosTimer;
        uint32 debilitatorDied;
        uint32 wrathOfEluneTimer;


        bool debilitated;
        bool attacked;

        void Reset() override
        {
            attacked = false;
        }

        void MovementInform(uint32 uiType, uint32 id) override
        {
            if (uiType != POINT_MOTION_TYPE)
                return;

            if (id == TYRANDE_PORTAL_CASTNG_WP)
            {
                PlaySimpleQuote(me, Tyrande::onOutOfArrows,false );
                me->CastSpell(me,SPELL_HAND_OF_ELUNE,false);
            }
            else if (id == PRELUDES_WP_ON_ENCOUNTER_FAIL)
            {
                me->SetFacingTo(tyrandeVarothenPos.m_orientation);
            }
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_TYRANDE_DEBILITATOR_DIED)
            {
                if (++debilitatorDied == 2)
                {
                    me->RemoveAura(SPELL_BLESSING_OF_ELUNE);
                    PlaySimpleQuote(me, Tyrande::onRelieved);
                    me->MonsterTextEmote("Tyrande can hold her own once again!", 0, true, 250.0f);
                    me->GetMotionMaster()->MovePoint(TYRANDE_PORTAL_CASTNG_WP, 3324.0f, -5707.0f, 16.3f, false, false);
                }
            }
            else if (action == ACTION_TYRANDE_MOVE_TO_VAROTHEN)
            {
                me->GetMotionMaster()->MovePoint(2, tyrandeVarothenPos, true, true);
                moveToCombatPosTimer = 6000;
            }
            else if (action == ACTION_TYRANDE_START_COMBAT_AFTER_WIPE)
            {
                moveToCombatPosTimer = 6000;
            }
            else if (action == ACTION_MANNOROTH_ENCOUNTER_FAILED)
            {
                debilitatorDied = 0;
                debilitated = false;
                me->AI()->EnterEvadeMode();
                me->GetMotionMaster()->MovePoint(PRELUDES_WP_ON_ENCOUNTER_FAIL, tyrandeVarothenPos, true, true);
            }
            else if (action == ACTION_MANNOROTH_FIGHT_ENDED)
            {
                me->InterruptNonMeleeSpells(false);
                me->RemoveAllAuras();
            }
            else if (action == ACTION_TYRANDE_PREPARE_WRATH_OF_ELUNE)
            {
                wrathOfEluneTimer = 14000 + 5000 + 8000;
            }
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell) override
        {
            if (spell->Id == SPELL_DEBILITATING_FLAY && debilitated == false)
            {
                me->CastSpell(me, SPELL_BLESSING_OF_ELUNE,false);
                debilitated = true;
                PlaySimpleQuote(me, Tyrande::onOverhelmed);
                me->MonsterTextEmote("Tyrande is overwhelmed! Use the Blessing of Elune to drive the demons back!", 0, true, 250.0f);
            }
        }

        void CastElunesWrath()
        {
            using namespace PortalDemons;
            uint32 creatures[5] = { FELHOUND, INFERNAL, DOOMGUARD_DEVASTATOR, FELGUARD, ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON };

            for (uint32 i = 0; i < 5; i++)
            {
                std::list<Creature*> lesserDemons;
                GetCreatureListWithEntryInGrid(lesserDemons, me, creatures[i], 55.0f);
                for (std::list<Creature*>::iterator itr = lesserDemons.begin(); itr != lesserDemons.end(); ++itr)
                    me->CastSpell((*itr),SPELL_ELUNES_WRATH,true);
                    // maybe use custom BP ?
            }
        }

        void EnterCombat(Unit* /*who*/) override { }
        void AttackStart(Unit* /*who*/) override { }
        void MoveInLineOfSight(Unit* /*who*/) override { }

        void UpdateAI(const uint32 diff) override
        {
            if (me->HasAura(SPELL_DEBILITATING_FLAY))
                return;

            if (wrathOfEluneTimer <= diff)
            {
                CastElunesWrath();
                wrathOfEluneTimer = MAX_TIMER;
            }
            else wrathOfEluneTimer -= diff;


            if (lunarShotTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false) && !me->HasAura(SPELL_HAND_OF_ELUNE))
                {
                    if (me->FindNearestCreature(ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON, 25.0f, true))
                    {
                        if (attacked == false)
                            PlaySimpleQuote(me,Tyrande::onAggro,false);

                        attacked = true;
                        me->CastSpell(me, SPELL_LUNAR_SHOT_AOE, false);
                    }
                    lunarShotTimer = 5000;
                }
            }
            else lunarShotTimer -= diff;

            if (moveToCombatPosTimer <= diff)
            {
                me->GetMotionMaster()->MovePoint(3, tyrandeCombatPos, true, true);
                lunarShotTimer = 2000;
                moveToCombatPosTimer = MAX_TIMER;
            }
            else moveToCombatPosTimer -= diff;
        }
    };
};

#define INVIS_MODEL_ID 11686

class npc_dreadlord_debilitator_woe : public CreatureScript
{
public:
    npc_dreadlord_debilitator_woe() : CreatureScript("npc_dreadlord_debilitator_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_dreadlord_debilitator_woeAI(creature);
    }

    struct npc_dreadlord_debilitator_woeAI : public ScriptedAI
    {
        npc_dreadlord_debilitator_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_DISABLE_MOVE);
            me->SetDisplayId(INVIS_MODEL_ID);
            delayedSpawnTimer = 7000;
            cosmeticTimer = 4000;
        }

        uint32 delayedSpawnTimer;
        uint32 cosmeticTimer;

        void Reset() override {}

        void EnterCombat(Unit* /*who*/) override { }
        void AttackStart(Unit* /*who*/) override { }
        void MoveInLineOfSight(Unit* /*who*/) override { }

        void JustDied(Unit *)
        {
            if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 250.0f, true))
                pTyrande->AI()->DoAction(ACTION_TYRANDE_DEBILITATOR_DIED);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (cosmeticTimer <= diff)
            {
                me->CastSpell(me, SPEL_DEBILITATOR_SPAWN_COSMETIC, true);
                cosmeticTimer = MAX_TIMER;
            }
            else cosmeticTimer -= diff;

            if (delayedSpawnTimer <= diff)
            {
                me->SetDisplayId(me->GetNativeDisplayId());

                if (Creature * pTyrande = me->FindNearestCreature(ENTRY_TYRANDE_PRELUDE, 250.0f, true))
                    me->CastSpell(pTyrande, SPELL_DEBILITATING_FLAY, false);

                delayedSpawnTimer = MAX_TIMER;
            }
            else delayedSpawnTimer -= diff;
        }
    };
};

class npc_fel_firestorm_trigger_woe : public CreatureScript
{
public:
    npc_fel_firestorm_trigger_woe() : CreatureScript("npc_fel_firestorm_trigger_woe") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_fel_firestorm_trigger_woeAI(creature);
    }

    struct npc_fel_firestorm_trigger_woeAI : public ScriptedAI
    {
        npc_fel_firestorm_trigger_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_DISABLE_MOVE);
            me->CastSpell(me, SPELL_FEL_FLAMES_AOE, true);
        }

        void KilledUnit(Unit * victim) override
        {
            if (Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 200.0f, true))
                pMannoroth->AI()->KilledUnit(victim);
        }

        void UpdateAI(const uint32 diff) override {}
    };
};

class npc_demon_portal_summoner : public CreatureScript
{
public:
    npc_demon_portal_summoner() : CreatureScript("npc_demon_portal_summoner") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_demon_portal_summonerAI(creature);
    }

    struct npc_demon_portal_summonerAI : public ScriptedAI
    {
        npc_demon_portal_summonerAI(Creature* creature) : ScriptedAI(creature)
        {
            me->AddAura(SPELL_NETHER_TEAR, me);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }

        uint32 felhoundTimer;
        uint32 felGuardTimer;
        uint32 devastatorTimer;

        uint64 mannorothGUID;

        void Reset() override
        {
            felhoundTimer = 1000;
            felGuardTimer = 2000;
            devastatorTimer = 3000;
            mannorothGUID = 0;

            if(Creature * pMannoroth = me->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true))
                mannorothGUID = pMannoroth->GetGUID();
        }

        void UpdateAI(const uint32 diff) override
        {
            if (felhoundTimer <= diff)
            {
                if (Creature * pMannoroth = ObjectAccessor::GetObjectInMap(mannorothGUID, me->GetMap(), (Creature*)NULL))
                {
                    if (pMannoroth->HealthBelowPct(85))
                        pMannoroth->SummonCreature(PortalDemons::FELHOUND, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                }
                felhoundTimer = 3000;
            }
            else felhoundTimer -= diff;

            if (felGuardTimer <= diff)
            {
                if (Creature * pMannoroth = ObjectAccessor::GetObjectInMap(mannorothGUID, me->GetMap(), (Creature*)NULL))
                {
                    if (pMannoroth->HealthBelowPct(70))
                        pMannoroth->SummonCreature(PortalDemons::FELGUARD, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                }
                felGuardTimer = 3000;
            }
            else felGuardTimer -= diff;

            if (devastatorTimer <= diff)
            {
                if (Creature * pMannoroth = ObjectAccessor::GetObjectInMap(mannorothGUID, me->GetMap(), (Creature*)NULL))
                {
                    if (pMannoroth->HealthBelowPct(60))
                        pMannoroth->SummonCreature(PortalDemons::DOOMGUARD_DEVASTATOR, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                }
                devastatorTimer = 3000;
            }
            else devastatorTimer -= diff;
        }
    };
};


class npc_portal_demon_woe : public CreatureScript
{
public:
    npc_portal_demon_woe() : CreatureScript("npc_portal_demon_woe") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_portal_demon_woeAI(creature);
    }

    struct npc_portal_demon_woeAI : public ScriptedAI
    {
        npc_portal_demon_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetInCombatWithZone();
            delayTimer = 3000;
            if (me->GetEntry() == PortalDemons::INFERNAL)
            {
                me->SetVisible(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            }
        }

        uint32 delayTimer;

        void AttackStart(Unit* unit) override { if (unit->GetTypeId() == TYPEID_PLAYER) ScriptedAI::AttackStart(unit); }

        void Reset() override
        {
            if (Unit * target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
            {
                if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
                {
                    me->AddThreat(target, 1.0f);
                    me->SetInCombatWith(target);
                    target->SetInCombatWith(me);
                    if (me->GetEntry() == PortalDemons::INFERNAL)
                    {
                        me->GetMotionMaster()->Clear(true);
                        me->GetMotionMaster()->MovementExpired(true);
                        me->GetMotionMaster()->MoveChase(target);
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (delayTimer <= diff)
            {
                if (me->GetEntry() == PortalDemons::INFERNAL)
                {
                    me->SetVisible(true);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
                    Reset();
                }
                delayTimer = MAX_TIMER;
            }
            else delayTimer -= diff;

            //Return since we have no target
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_twisting_nether_portal_woe : public CreatureScript
{
public:
    npc_twisting_nether_portal_woe() : CreatureScript("npc_twisting_nether_portal_woe") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_twisting_nether_portal_woeAI(creature);
    }

    struct npc_twisting_nether_portal_woeAI : public ScriptedAI
    {
        npc_twisting_nether_portal_woeAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            demonGUIDs.clear();
            pInstance = me->GetInstanceScript();
        }

        InstanceScript * pInstance;
        std::vector<uint64> demonGUIDs;

        uint32 pullTimer = MAX_TIMER;
        uint32 summonAnnihilatorTimer = MAX_TIMER;
        bool pull_started = false;
        bool pull_ended = false;

        void SetGUID(const uint64 & guid, int32 id = 0) override
        {
            if (pull_started == false)
            {
                pullTimer = 3000;
                pull_started = true;
            }

            uint64 demonGUID = guid;
            demonGUIDs.push_back(demonGUID);
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_START_SUMMON_DEVASTATORS)
            {
                summonAnnihilatorTimer = 2000;
            }
            else if (action == ACTION_STOP_SUMMON_DEVASTATORS)
            {
                summonAnnihilatorTimer = MAX_TIMER;
            }
        }

        void EnterCombat(Unit*) override { }
        void AttackStart(Unit*) override { }
        void EnterEvadeMode() override {}

        void UpdateAI(const uint32 diff) override
        {
            if (!pInstance)
                return;

            if (pull_ended == false && pullTimer <= diff && !demonGUIDs.empty())
            {
                if (Creature * pDemon = ObjectAccessor::GetObjectInMap(demonGUIDs[0], me->GetMap(), (Creature*)NULL))
                {
                    pDemon->ForcedDespawn(15000);
                    pDemon->GetMotionMaster()->Clear(false);
                    pDemon->GetMotionMaster()->MovementExpired(false);
                    pDemon->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 20.0f, 20.0f);
                    demonGUIDs.erase(demonGUIDs.begin());
                }

                if (demonGUIDs.empty())
                    pull_ended = true;

                pullTimer = 500;
            }
            else pullTimer -= diff;

            if (summonAnnihilatorTimer <= diff)
            {
                uint32 demonsCount = urand(4, 5);

                for (uint32 i = 0; i < demonsCount; i++)
                {
                    if (Creature* pMannoroth = ObjectAccessor::GetCreature(*me, pInstance->GetData64(DATA_MANNOROTH)))
                    {
                        Creature * pAnnihilator = pMannoroth->SummonCreature(ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 4.1f);

                        float xDiff = float(urand(0, 12));
                        float yDiff = float(urand(0, 5));

                        if (urand(0, 1))
                            xDiff *= -1.0f;

                        if (urand(0, 1))
                            yDiff *= -1.0f;

                        // TODO: Improve land point positions
                        if (pAnnihilator)
                            pAnnihilator->GetMotionMaster()->MovePoint(0, 3296.0f + xDiff, -5671.0f + yDiff, 12.0f, false, false);
                    }
                }
                summonAnnihilatorTimer = 15000;
            }
            else summonAnnihilatorTimer -= diff;
        }
    };
};

class npc_doomgurad_annihilator_summon : public CreatureScript
{
public:
    npc_doomgurad_annihilator_summon() : CreatureScript("npc_doomgurad_annihilator_summon") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_doomgurad_annihilator_summonAI(creature);
    }

    struct npc_doomgurad_annihilator_summonAI : public ScriptedAI
    {
        npc_doomgurad_annihilator_summonAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override {}
        void AttackStart(Unit* unit) override { if (unit->GetTypeId() == TYPEID_PLAYER) ScriptedAI::AttackStart(unit);}

        void JustDied(Unit *) override { me->ForcedDespawn(30000);}

        void MovementInform(uint32 uiType, uint32 uiId)
        {
            if (uiType != POINT_MOTION_TYPE)
                return;

            if (uiId == 0)
            {
                me->SetFlying(false);
                me->RemoveUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING);
                me->SetReactState(REACT_AGGRESSIVE);
                me->SetInCombatWithZone();
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };
};

/**********************************************    SPELL SCRIPTS    ***************************************************************/

class spell_nether_portal_woe : public SpellScriptLoader
{
    public:
        spell_nether_portal_woe() : SpellScriptLoader("spell_nether_portal_woe") { }

        class spell_nether_portal_woe_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_nether_portal_woe_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList.clear();

                std::list<Creature*> debilitators;
                GetCaster()->GetCreatureListWithEntryInGrid(debilitators, ENTRY_DDREADLORD_DEBILITATOR, 250.0f);
                for (std::list<Creature*>::iterator itr = debilitators.begin(); itr != debilitators.end(); ++itr)
                    unitList.push_back((*itr));
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_nether_portal_woe_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_nether_portal_woe_SpellScript();
        }
};

class spell_tyrande_lunar_shot : public SpellScriptLoader
{
public:
    spell_tyrande_lunar_shot() : SpellScriptLoader("spell_tyrande_lunar_shot") { }

    class spell_tyrande_lunar_shot_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_tyrande_lunar_shot_SpellScript);

        void FilterTargets(std::list<Unit*>& unitList)
        {
            unitList.clear();

            std::list<Creature*> annihilators;
            GetCaster()->GetCreatureListWithEntryInGrid(annihilators, ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON, 50.0f);

            for (std::list<Creature*>::iterator itr = annihilators.begin(); itr != annihilators.end(); ++itr)
                unitList.push_back((*itr));
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_tyrande_lunar_shot_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_tyrande_lunar_shot_SpellScript();
    }
};

class spell_tyrande_wrath_of_elune : public SpellScriptLoader
{
public:
    spell_tyrande_wrath_of_elune() : SpellScriptLoader("spell_tyrande_wrath_of_elune") { }

    class spell_tyrande_wrath_of_elune_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_tyrande_wrath_of_elune_SpellScript);

        void FilterTargets(std::list<Unit*>& unitList)
        {
            unitList.clear();

            std::list<Unit*> searchTargets;
            Trinity::AnyUnitInObjectRangeCheck u_check(GetCaster(), 100.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(GetCaster(), searchTargets, u_check);
            GetCaster()->VisitNearbyObject(100.0f, searcher);
            
            for (std::list<Unit*>::iterator itr = searchTargets.begin(); itr != searchTargets.end(); itr++)
            {
                Unit * unit = *itr;

                if (unit->GetTypeId() == TYPEID_UNIT)
                {
                    using namespace PortalDemons;

                    uint32 entry = unit->GetEntry();
                    if (entry == FELHOUND || entry == FELGUARD || entry == DOOMGUARD_DEVASTATOR || entry == ENTRY_DOOMGUARD_ANNIHILATOR_SUMMON)
                    {
                        unitList.push_back(unit);
                    }
                }

            }

            if (unitList.size() > 3)
                unitList.resize(3);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_tyrande_wrath_of_elune_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_tyrande_wrath_of_elune_SpellScript();
    }
};

class spell_demon_portal_pull_visual : public SpellScriptLoader
{
public:
    spell_demon_portal_pull_visual() : SpellScriptLoader("spell_demon_portal_pull_visual") { }

    class spell_demon_portal_pull_visual_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_demon_portal_pull_visual_SpellScript);

        void FilterTargets(std::list<Unit*>& targets)
        {
            targets.clear();
            std::list<Unit*> unitList;

            Creature * caster = GetCaster()->ToCreature();

            if (caster == nullptr)
                return;

            Trinity::AnyUnitInObjectRangeCheck u_check(caster, 250.0f);
            Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(caster, unitList, u_check);
            caster->VisitNearbyObject(300.0f, searcher);

            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); itr++)
            {
                Unit * unit = *itr;
                uint32 entry = unit->GetEntry();

                if (unit && unit->GetTypeId() == TYPEID_UNIT) // only creatures
                {
                    if (unit->IsAlive() && unit->GetOwnerGUID() == 0) // only live and no pets
                    if (unit->GetCreatureType() == CREATURE_TYPE_DEMON) // only demons
                    {
                        if (entry != MANNOROTH_ENTRY && entry != ENTRY_MANNOROTH_VEHICLE_RIDER && entry != LEGION_PORTAL_DEMON)
                        {
                            targets.push_back(unit);

                            caster->AI()->SetGUID(unit->GetGUID());
                            unit->AddAura(SPELL_DEMON_STUN, unit);
                            unit->GetMotionMaster()->Clear(false);
                            unit->GetMotionMaster()->MovementExpired(false);
                            unit->InterruptNonMeleeSpells(false);
                        }
                    }
                }
            }
            // Add mannoroth vehicle riders
            if (Creature * pMannoroth = caster->FindNearestCreature(MANNOROTH_ENTRY, 250.0f, true))
            {
                if (Vehicle * veh = pMannoroth->GetVehicleKit())
                {
                    uint8 seats = veh->m_Seats.size();

                    for (uint8 i = 0; i < seats; i++)
                    {
                        if (Unit * passenger = veh->GetPassenger(i))
                        {
                            targets.push_back(passenger);
                        }
                    }
                }
            }
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_demon_portal_pull_visual_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_demon_portal_pull_visual_SpellScript();
    }
};

class spell_inferno_mannoroth : public SpellScriptLoader
{
public:
    spell_inferno_mannoroth() : SpellScriptLoader("spell_inferno_mannoroth") { }

    class spell_inferno_mannoroth_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_inferno_mannoroth_SpellScript);

        void SummonInfernal(SpellEffIndex /*effIndex*/)
        {
            WorldLocation* misssilePos = GetTargetDest();
            Unit * caster = GetCaster();

            caster->SummonCreature(PortalDemons::INFERNAL, misssilePos->GetPositionX(), misssilePos->GetPositionY(), misssilePos->GetPositionZ(), misssilePos->GetOrientation());
        }

        void Register()
        {
            OnEffect += SpellEffectFn(spell_inferno_mannoroth_SpellScript::SummonInfernal, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_inferno_mannoroth_SpellScript();
    }
};

class spell_gen_gift_of_sargeras_woe : public SpellScriptLoader
{
public:
    spell_gen_gift_of_sargeras_woe() : SpellScriptLoader("spell_gen_gift_of_sargeras_woe") { }

    class spell_gen_gift_of_sargeras_woe_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_gen_gift_of_sargeras_woe_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = aurEff->GetCaster();
            if (!caster || !caster->GetInstanceScript())
                return;

            caster->GetInstanceScript()->DoAddAuraOnPlayers(nullptr, SPELL_GIFT_OF_SARGERAS_INSTANT);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_gen_gift_of_sargeras_woe_AuraScript::OnApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_gen_gift_of_sargeras_woe_AuraScript();
    }
};

void AddSC_boss_mannoroth()
{
    new boss_mannoroth_woe();
    new boss_captain_varothen_woe();
    new npc_varothen_magic_blade(); // 55837

    new npc_tyrande_mannoroth_woe();
    new npc_fel_firestorm_trigger_woe();
    new npc_dreadlord_debilitator_woe();
    new npc_portal_demon_woe();             // 56001,56002,55739,56036
    new npc_demon_portal_summoner();        // 55839
    new npc_twisting_nether_portal_woe(); // 56087
    new npc_doomgurad_annihilator_summon(); // 55700

    new spell_nether_portal_woe();          // 104625 + 104648
    new spell_demon_portal_pull_visual();   // 105531
    new spell_inferno_mannoroth(); // 105145
    new spell_tyrande_lunar_shot(); // 104688
    new spell_tyrande_wrath_of_elune(); // 105073, 105075
    new spell_gen_gift_of_sargeras_woe(); // 104998
}

// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104625, 'spell_nether_portal_woe');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104648 , 'spell_nether_portal_woe');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105531 , 'spell_demon_portal_pull_visual');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105145, 'spell_inferno_mannoroth');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104688, 'spell_tyrande_lunar_shot');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105073, 'spell_tyrande_wrath_of_elune');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (105075, 'spell_tyrande_wrath_of_elune');
// INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (104998, 'spell_gen_gift_of_sargeras_woe');
