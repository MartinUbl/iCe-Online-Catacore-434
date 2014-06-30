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
#include <random>
#include <algorithm>

struct Quotes
{
    uint32 soundId;
    const char * text;
};

static const Quotes firstIntroQuotes[6] = 
{
    { 26282, "No mortal shall turn me from my task!" }, // Morchok
    { 26531, "Wyrmrest Accord, attack!" }, // Lord Afrasastrasz
    { 26305, "They have broken our defenses! The very earth turns against us in Deathwing's name." }, // Image of Tyrygosa
    { 26306, "You must hurry...Wyrmrest falls as we speak. All...is lost." }, // Image of Tyrygosa
    { 26532, "Tyrygosa yet lives! We must press on, to the Temple!" }, // Lord Afrasastrasz
    { 26270, "Cowards! Weaklings! Come down and fight or I will bring you down!" }, // Morchok
};

static const Quotes secondIntroQuotes[5] =
{
    { 26271, "You cannot hide in this temple forever, shaman!" }, // Morchok
    { 26273, "Wyrmrest will fall. All will be dust." }, // Morchok
    { 26533, "Advance to the front!" }, // Lord Afrasastrasz
    { 26534, "The siege must be broken! Wyrmrest Accord, defend the line!" }, // Lord Afrasastrasz
    { 26272, " I will turn this tower to rubble and scatter it across the wastes." }, // Morchok
};

static const Quotes aggroQuote = { 26268, "You seek to halt an avalanche. I will bury you." };
 
static const Quotes summonQuotes[3] =
{
    // Summon Kohcrom
    { 26288, "You thought to fight me alone? The earth splits to swallow and crush you." },
    // Summon Resonating Crystal
    { 26283, "Flee and die!" },
    { 26284, "Run, and perish." },
};

static const Quotes vortexQuotes[4] =
{
    { 26274, "The stone calls..." },
    { 26275, "The ground shakes..." },
    { 26276, "The rocks tremble..." },
    { 26277, "The surface quakes..." },
};

static const Quotes blackBloodQuotes[4] =
{
    { 26278, "...and the black blood of the earth consumes you." },
    { 26279, "...and there is no escape from the old gods." },
    { 26280, "...and the rage of the true gods follows." },
    { 26281, "...and you drown in the hate of The Master." },
};

static const Quotes killQuotes[3] =
{
    { 26285, "I am unstoppable." },
    { 26286, "It was inevitable." },
    { 26287, "Ground to dust." },
};

static const Quotes deathQuotes[2] =
{
    { 26269, "Impossible. This cannot be. The tower...must...fall." }, // Morchok
    { 26535, "The Twilight's Hammer is retreating! The temple is ours; fortify your positions within!" }, // Lord Afasastrasz
};


enum Spells
{
    SPELL_STOMP = 103414,
    SPELL_CRUSH_ARMOR = 103687,
    SPELL_RESONATING_CRYSTAL_MISSILE = 103640, // missile which spawn crystal on players nearby location
    SPELL_FURIOUS = 103846,
    SPELL_EARTH_VENGEANCE = 103176, // A.K.A Falling Fragment(s) SPELL_AURA_PERIODIC_TRIGGER_SPELL -> TODO: spawn GOs on tick
    SPELL_EARTH_VORTEX = 103821, // (pull) teleport players in front of the boss + summon npc ( Vehicle ??? )
    SPELL_EARTH_VORTEX_VEHICLE_AURA = 109615,
    SPELL_FALLING_FRAGMENT_MISSILE = 103177, // spawn fragment
    SPELL_BLACK_BLOOD_VISUAL = 103180, // black fluid viusal on target ( only for 6 seconds cca)
    SPELL_BLACK_BLOOD_CHANNEL = 103851, // trigger SPELL_BLACK_BLOOD_DAMAGE periodicaly
    SPELL_BLACK_BLOOD_DAMAGE = 103785, // radius must be calculated manually
};

enum resonatingCrystalSpells
{
    SPELL_CRYSTAL_DAMAGE = 103545,
    SPELL_CRYSTAL_VISUAL = 103494,

    // Beams signaling distance from crystal
    DANGER_BEAM = 103534,
    WARNING_BEAM = 103536,
    SAFE_BEAM = 103541
};

enum creatureEntries
{
    MORCHOK_ENTRY = 55265,
    KOHCROM_ENTRY = 57773, // Heroic twin 
    EARTHEN_VORTEX_ENTRY = 55723, // A.K.A  Black blood
    RESONATING_CRYSTAL_ENTRY = 55346,
    EARTHEN_VORTEX_VEHICLE_ENTRY = 55723
};

enum actions
{
    ACTIONS_START = 1,
    ACTION_SPAWN_FRAGMENT = 1000, // just high enough
    ACTION_CAST_BLACK_BLOOD = 1001,
};

#define FRAGMENT_GO_ENTRY  (209596)

# define NEVER  (0xffffffff)

class boss_morchok_dragon_soul : public CreatureScript
{
public:
    boss_morchok_dragon_soul() : CreatureScript("boss_morchok_dragon_soul") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_morchok_dragon_soulAI(creature);
    }

    struct boss_morchok_dragon_soulAI : public ScriptedAI
    {
        boss_morchok_dragon_soulAI(Creature* creature) : ScriptedAI(creature), Summons(creature)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        SummonList Summons;
        // Timers
        uint32 stompTimer;
        uint32 crushArmorTimer;
        uint32 crystalTimer;
        uint32 fragmentTimer;

        bool furious;
        bool canCastBlackBlood;

        uint32 bloodTicks;

        void JustSummoned(Creature* pSummoned)
        {
            Summons.Summon(pSummoned);
        }

        void Reset()
        {
            bloodTicks = 0;

            crushArmorTimer = 6000;
            stompTimer = 12000;
            crystalTimer = 18000;
            fragmentTimer = 56000;
            furious = false;
            canCastBlackBlood = false;

            Summons.DespawnAll();
        }

        void PlayAndYell(uint32 soundId, const char * text)
        {
            DoPlaySoundToSet(me, soundId);
            me->MonsterYell(text, LANG_UNIVERSAL, 0);
        }

        void DespawnFragments(void)
        {
            std::list<GameObject*> objects;
            me->GetGameObjectListWithEntryInGrid(objects, FRAGMENT_GO_ENTRY, 100.0f);
            for (std::list<GameObject*>::iterator itr = objects.begin(); itr != objects.end(); ++itr)
                (*itr)->Delete();
        }

        void RemoveBeamAurasFromPlayers()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                    p->RemoveAurasDueToSpell(SAFE_BEAM);
                    p->RemoveAurasDueToSpell(WARNING_BEAM);
                    p->RemoveAurasDueToSpell(DANGER_BEAM);
                }
            }
        }

        void ApplyVortexAuraOnPlayers()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                    if (Aura * a = me->AddAura(SPELL_EARTH_VORTEX_VEHICLE_AURA, p))
                        a->SetDuration(5000);
                }
            }
        }

        void DoAction(const int32 action)
        {
            if (action == ACTION_SPAWN_FRAGMENT)
            {
                float angle = frand(0, 2 * M_PI);
                const uint32 distance = urand(35, 42);
                float x = me->GetPositionX() + cos(angle)*distance;
                float y = me->GetPositionY() + sin(angle)*distance;
                float z = me->GetMap()->GetHeight(me->GetPhaseMask(), x, y, me->GetPositionZ() + 10.0f, true);

                me->CastSpell(x, y, z, SPELL_FALLING_FRAGMENT_MISSILE, true);
            }
            else if (action == ACTION_CAST_BLACK_BLOOD)
            {
                bloodTicks = 0;
                canCastBlackBlood = true;
            }
            else // Ticks from black blood
            {
                bloodTicks++;
                uint32 bloods = (2 * M_PI * action);
                float angle = 0.0f;

                for (uint32 i = 0; i < bloods; i++)
                {
                    angle += 6.28f / bloods;
                    angle = MapManager::NormalizeOrientation(angle);
                    const uint32 distance = bloodTicks * 10;
                    if (distance >= 100.0f)
                        break;
                    float x = me->GetPositionX() + cos(angle)*distance;
                    float y = me->GetPositionY() + sin(angle)*distance;
                    float z = me->GetMap()->GetHeight(me->GetPhaseMask(), x, y, me->GetPositionZ() + 10.0f, true);
                    // Dont spawn blood behind fragments
                    if (me->IsWithinLOS(x,y,z))
                        me->SummonCreature(EARTHEN_VORTEX_ENTRY, x, y, z, 0.0f,TEMPSUMMON_MANUAL_DESPAWN,0);
                }
            }
        }

        void EnterCombat(Unit * /*who*/)
        {
            PlayAndYell(aggroQuote.soundId, aggroQuote.text);
        }

        void EnterEvadeMode()
        {
            RemoveBeamAurasFromPlayers();
            DespawnFragments();
            ScriptedAI::EnterEvadeMode();
        }

        void KilledUnit(Unit* victim)
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 randInt = urand(0, 2);
                PlayAndYell(killQuotes[randInt].soundId, killQuotes[randInt].text);
            }
        }

        void JustDied()
        {
            uint32 randInt = urand(0, 1);
            PlayAndYell(deathQuotes[randInt].soundId, deathQuotes[randInt].text);

            RemoveBeamAurasFromPlayers();
            DespawnFragments();
            Summons.DespawnAll();
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (canCastBlackBlood)
            {
                if (me->IsNonMeleeSpellCasted(false))
                    return;

                uint32 randInt = urand(0, 3);
                PlayAndYell(blackBloodQuotes[randInt].soundId, blackBloodQuotes[randInt].text);

                // Start channeling after summonning of fragments
                me->CastSpell(me, SPELL_BLACK_BLOOD_CHANNEL, false);
                canCastBlackBlood = false;

                return;
            }

            if (crushArmorTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me->getVictim(), SPELL_CRUSH_ARMOR, false);
                    crushArmorTimer = urand(6000, 15000);
                }
            }
            else crushArmorTimer -= diff;

            if (stompTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    me->CastSpell(me, SPELL_STOMP, false);
                    stompTimer = 14000;
                }
            }
            else stompTimer -= diff;

            if (crystalTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    uint32 randInt = urand(1, 2);
                    PlayAndYell(summonQuotes[randInt].soundId, summonQuotes[randInt].text);

                    // TODO : Change second parameter to 1 after release of AI
                    if (Unit * plr = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        me->CastSpell(plr, SPELL_RESONATING_CRYSTAL_MISSILE, true);

                    crystalTimer = 16000;
                }
            }
            else crystalTimer -= diff;

            if (fragmentTimer <= diff)
            {
                if (!me->IsNonMeleeSpellCasted(false))
                {
                    uint32 randInt = urand(0, 3);
                    PlayAndYell(vortexQuotes[randInt].soundId, vortexQuotes[randInt].text);

                    me->CastSpell(me, SPELL_EARTH_VORTEX, false); // pull players
                    ApplyVortexAuraOnPlayers();
                    me->CastSpell(me, SPELL_EARTH_VENGEANCE, false); // start summoning fragments
                    fragmentTimer = 97000;
                }
            }
            else fragmentTimer -= diff;

            if (me->HealthBelowPct(20) && !furious)
            {
                furious = true;
                me->CastSpell(me, SPELL_FURIOUS, true);
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_resonating_crystal : public CreatureScript
{
public:
    npc_ds_resonating_crystal() : CreatureScript("npc_ds_resonating_crystal") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ds_resonating_crystalAI(creature);
    }

    struct npc_ds_resonating_crystalAI : public ScriptedAI
    {
        npc_ds_resonating_crystalAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
        }

        void RemoveBeamAuraFromPlayers()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            if (plList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                    p->RemoveAurasDueToSpell(SAFE_BEAM);
                    p->RemoveAurasDueToSpell(WARNING_BEAM);
                    p->RemoveAurasDueToSpell(DANGER_BEAM);
                }
            }
        }

        void SelectBeamTargets()
        {
            Map::PlayerList const& plList = me->GetMap()->GetPlayers();

            if (plList.isEmpty())
                return;

            std::vector<Player*> beamTargets;

            for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
            {
                if (Player * p = itr->getSource())
                {
                    if (!p->HasTankSpec() && !p->HasAura(5487) // Bear form
                        && !p->isGameMaster() && p->isAlive())
                    {
                        beamTargets.push_back(p);
                    }
                }
            }

            if (beamTargets.empty())
                return;

            std::random_shuffle(beamTargets.begin(), beamTargets.end()); // Random shuffle non tanky players

            uint32 max = Is25ManRaid() ? 7 : 3;
            uint32 vectorSize = beamTargets.size();
            max = vectorSize < max ? vectorSize : max;

            for (uint32 i = 0; i < max; i++)
            {
                if (beamTargets[i] && beamTargets[i]->IsInWorld())
                {
                    beamGUIDS.push_back(beamTargets[i]->GetGUID());
                    me->CastSpell(beamTargets[i], GetBeamSpellId(me->GetDistance(beamTargets[i])), true);
                }
            }
        }

        uint32 GetBeamSpellId(float distance)
        {
            if (distance < 5.0f)
                return SAFE_BEAM;

            if (distance < 20.0f)
                return WARNING_BEAM;

            return DANGER_BEAM;
        }

        uint32 explodeTimer;
        uint32 beamTimer;
        uint32 sizeTimer;
        uint32 refreshTimer;
        InstanceScript * instance;

        std::list<uint64> beamGUIDS;

        void Reset()
        {
            beamGUIDS.clear();
            explodeTimer = 12000;
            beamTimer = 2000;
            refreshTimer = beamTimer + 1000;
            sizeTimer = 3000;
            me->SetReactState(REACT_PASSIVE);
            me->CastSpell(me, SPELL_CRYSTAL_VISUAL, true);
        }

        void EnterEvadeMode() {}

        void UpdateAI(const uint32 diff)
        {
            if (refreshTimer <= diff)
            {
                for (std::list<uint64>::iterator it = beamGUIDS.begin(); it != beamGUIDS.end(); it++)
                {
                    if (Player* beamedPlayer = ObjectAccessor::GetPlayer(*me, *it))
                    {
                        if (beamedPlayer->HasAura(GetBeamSpellId(me->GetDistance(beamedPlayer))))
                            continue;

                        beamedPlayer->RemoveAurasDueToSpell(SAFE_BEAM);
                        beamedPlayer->RemoveAurasDueToSpell(WARNING_BEAM);
                        beamedPlayer->RemoveAurasDueToSpell(DANGER_BEAM);

                        me->CastSpell(beamedPlayer, GetBeamSpellId(me->GetDistance(beamedPlayer)), true);
                    }
                }
                refreshTimer = 1000;
            }
            else refreshTimer -= diff;

            if (explodeTimer <= diff)
            {
                me->CastSpell(me, SPELL_CRYSTAL_DAMAGE, false);
                explodeTimer = NEVER;
                me->RemoveAllAuras();
                RemoveBeamAuraFromPlayers();
                refreshTimer = 2000; // Prevent applying of beam after explosion ( need 1 second to play spell animation and than despawn)
                me->ForcedDespawn(1000);
            }
            else explodeTimer -= diff;

            if (beamTimer <= diff)
            {
                SelectBeamTargets(); // Mark players with beams
                beamTimer = NEVER;
            }
            else beamTimer -= diff;

            if (sizeTimer <= diff) // Shards should shrink every few seconds
            {
                if (AuraEffect * aurEff = me->GetAuraEffect(SPELL_CRYSTAL_VISUAL, EFFECT_1))
                {
                    if (aurEff->GetAmount() >= 50)
                    {
                        aurEff->SetAmount(aurEff->GetAmount() - 50);
                        aurEff->SetCanBeRecalculated(true);
                        aurEff->RecalculateAmount();
                    }
                }
                sizeTimer = 1500;
            }
            else sizeTimer -= diff;
        }
    };
};

class npc_ds_earthen_vortex : public CreatureScript
{
public:
    npc_ds_earthen_vortex() : CreatureScript("npc_ds_earthen_vortex") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ds_earthen_vortexAI(creature);
    }

    struct npc_ds_earthen_vortexAI : public ScriptedAI
    {
        npc_ds_earthen_vortexAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            me->ForcedDespawn(14000); // Default despawn timer
        }

        uint32 bloodVisualTimer;

        void Reset()
        {
            bloodVisualTimer = 5000;
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterEvadeMode() {}

        void UpdateAI(const uint32 diff)
        {
            if (bloodVisualTimer <= diff)
            {
                me->CastSpell(me, SPELL_BLACK_BLOOD_VISUAL, false); // Need to refresh visual effect cause it only last cca 5 seconds
                bloodVisualTimer = 4000;
            }
            else bloodVisualTimer -= diff;
        }
    };
};

class spell_ds_resonating_blast : public SpellScriptLoader
{
public:
    spell_ds_resonating_blast() : SpellScriptLoader("spell_ds_resonating_blast") {}

    class spell_ds_resonating_blast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_resonating_blast_SpellScript);

        int32 hit_targets;

        bool Load()
        {
            hit_targets = 0;
            return true;
        }

        void FilterTargets(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            unitList.sort(Trinity::ObjectDistanceOrderPred(caster));

            uint32 listSize = unitList.size();

            if (GetSpellInfo()->Id == 103545 || GetSpellInfo()->Id == 110041) // 10 man
                unitList.resize(listSize < 3 ? listSize : 3);
            else
                unitList.resize(listSize < 3 ? listSize : 7);

            hit_targets = unitList.size();


            /*for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
            {
                if ((*itr)->HasAura(SAFE_BEAM) || (*itr)->HasAura(WARNING_BEAM) || (*itr)->HasAura(DANGER_BEAM))
                {
                    ++hit_targets;
                    itr++;
                }
                else
                    itr = unitList.erase(itr);
            }*/
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * caster = GetCaster();
            Unit * hit_unit = GetHitUnit();

            if (!caster || !hit_unit || hit_targets == 0)
                return;

            int32 damage = GetHitDamage() / hit_targets; // Split damage between beam targets
            float distance = caster->GetDistance(hit_unit);

            // The total damage increases the further the targets are from the explosion.
            if (distance < 5.0f)
                damage = damage * 1;
            else if (distance < 20.0f)
                damage = damage * 2;
            else
                damage = damage * 3;

            SetHitDamage((int32)damage);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_resonating_blast_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_resonating_blast_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_resonating_blast_SpellScript();
    }
};

class spell_ds_morchok_stomp : public SpellScriptLoader
{
public:
    spell_ds_morchok_stomp() : SpellScriptLoader("spell_ds_morchok_stomp") {}

    class spell_ds_morchok_stomp_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_morchok_stomp_SpellScript);

        uint64 guids[2];
        uint32 hitUnits;

        bool Load()
        {
            hitUnits = 0;
            guids[0] = 0;
            guids[1] = 0;
            return true;
        }

        void GetClosestTargets(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            unitList.sort(Trinity::ObjectDistanceOrderPred(caster));

            int32 max = 2;

            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
            {
                max--;
                if (max < 0)
                    break;

                guids[(uint32)max] = (*itr)->GetGUID();
            }

            hitUnits = unitList.size();
        }

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit * hit_unit = GetHitUnit();

            if (!hit_unit || hitUnits == 0)
                return;

            int32 damage = GetHitDamage() / hitUnits; // Share damage

            // The two closest targets take a double share of the damage
            if (hit_unit->GetGUID() == guids[0] || hit_unit->GetGUID() == guids[1])
                SetHitDamage(damage * 2);
            else
                SetHitDamage(damage);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_morchok_stomp_SpellScript::GetClosestTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnEffect += SpellEffectFn(spell_ds_morchok_stomp_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_morchok_stomp_SpellScript();
    }
};


class spell_ds_summon_fragments : public SpellScriptLoader
{
public:
    spell_ds_summon_fragments() : SpellScriptLoader("spell_ds_summon_fragments") { }

    class spell_ds_summon_fragments_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_summon_fragments_AuraScript);

        void HandleEffectPeriodic(AuraEffect const * aurEff)
        {
            Unit *caster = aurEff->GetBase()->GetCaster();
            if (!caster || !caster->ToCreature())
                return;

            caster->ToCreature()->AI()->DoAction(ACTION_SPAWN_FRAGMENT);

            if (urand(0,100) > 70) // There are some more fragments on blizz servers
                caster->ToCreature()->AI()->DoAction(ACTION_SPAWN_FRAGMENT);

            // Immediately cast black blood after spawning all fragments
            if (aurEff->GetTickNumber() == aurEff->GetTotalTicks())
                caster->ToCreature()->AI()->DoAction(ACTION_CAST_BLACK_BLOOD);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_summon_fragments_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_ds_summon_fragments_AuraScript();
    }
};

class spell_ds_summon_black_blood : public SpellScriptLoader
{
public:
    spell_ds_summon_black_blood() : SpellScriptLoader("spell_ds_summon_black_blood") { }

    class spell_ds_summon_black_blood_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_summon_black_blood_AuraScript);

        void HandleEffectPeriodic(AuraEffect const * aurEff)
        {
            Unit *caster = aurEff->GetBase()->GetCaster();
            if (!caster || !caster->ToCreature())
                return;

            caster->ToCreature()->AI()->DoAction(ACTIONS_START + aurEff->GetTickNumber()); // start +  tick offset

            // Make fragments untargetable and enable los collision
            if (aurEff->GetTickNumber() == 0 || aurEff->GetTickNumber() == 1) // Not sure if first tick is 0 or 1
            {
                std::list<GameObject*> objects;
                caster->GetGameObjectListWithEntryInGrid(objects, FRAGMENT_GO_ENTRY, 100.0f);
                for (std::list<GameObject*>::iterator itr = objects.begin(); itr != objects.end(); ++itr)
                {
                    (*itr)->EnableCollision(true);
                    (*itr)->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                }
            }

            // Destroy fragments + black blood after last tick of channeling of Black Blood
            if (aurEff->GetTickNumber() == aurEff->GetTotalTicks())
            {
                std::list<Creature*> vortexes;
                caster->GetCreatureListWithEntryInGrid(vortexes, EARTHEN_VORTEX_ENTRY, 500.0f);
                for (std::list<Creature*>::iterator itr = vortexes.begin(); itr != vortexes.end(); ++itr)
                    (*itr)->ForcedDespawn();

                std::list<GameObject*> objects;
                caster->GetGameObjectListWithEntryInGrid(objects, FRAGMENT_GO_ENTRY, 100.0f);
                for (std::list<GameObject*>::iterator itr = objects.begin(); itr != objects.end(); ++itr)
                    (*itr)->Delete();
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_ds_summon_black_blood_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_ds_summon_black_blood_AuraScript();
    }
};

class spell_ds_black_blood_damage : public SpellScriptLoader
{
public:
    spell_ds_black_blood_damage() : SpellScriptLoader("spell_ds_black_blood_damage") {}

    class spell_ds_black_blood_damage_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_black_blood_damage_SpellScript);

        void RemoveFarTargets(std::list<Unit*>& unitList)
        {
            Unit * caster = GetCaster();
            if (!caster)
                return;

            boss_morchok_dragon_soul::boss_morchok_dragon_soulAI* pAI = (boss_morchok_dragon_soul::boss_morchok_dragon_soulAI*)(caster->GetAI());
            
            for (std::list<Unit*>::iterator itr = unitList.begin(); itr != unitList.end();)
            {
                if (pAI)
                {
                    if (caster->GetDistance(*itr) > (3.0f * pAI->bloodTicks) || !(*itr)->IsWithinLOSInMap(caster))
                    {
                        itr = unitList.erase(itr);
                    }
                    else
                        itr++;
                }
            }
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_black_blood_damage_SpellScript::RemoveFarTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_DST);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_black_blood_damage_SpellScript::RemoveFarTargets, EFFECT_1, TARGET_UNIT_AREA_ENEMY_DST);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_black_blood_damage_SpellScript();
    }
};


void AddSC_boss_morchok()
{
    //Creature scripts
    new boss_morchok_dragon_soul(); // 55265
    new npc_ds_resonating_crystal(); // 55346
    new npc_ds_earthen_vortex(); // 55723

    // Spell/AuraScripts
    new spell_ds_summon_fragments(); // 103176
    new spell_ds_summon_black_blood(); // 103851
    new spell_ds_black_blood_damage(); // 103785, 108570, 110288, 110287
    new spell_ds_resonating_blast(); // 103545, 108572, 110041, 110040
    new spell_ds_morchok_stomp(); // 103414, 108571, 109033, 109034
}

/*
    UPDATE `creature_template` SET `ScriptName`='boss_morchok_dragon_soul' WHERE  `entry`=55265 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='npc_ds_resonating_crystal' WHERE  `entry`=55346 LIMIT 1;
    UPDATE `creature_template` SET `ScriptName`='npc_ds_earthen_vortex' WHERE  `entry`=55723 LIMIT 1;

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103176, 'spell_ds_summon_fragments');

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES (103851, 'spell_ds_summon_black_blood');

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103785, 'spell_ds_black_blood_damage'),
    (108570, 'spell_ds_black_blood_damage'),
    (110288, 'spell_ds_black_blood_damage'),
    (110287, 'spell_ds_black_blood_damage');

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103545, 'spell_ds_resonating_blast'),
    (108572, 'spell_ds_resonating_blast'),
    (110041, 'spell_ds_resonating_blast'),
    (110040, 'spell_ds_resonating_blast');

    INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (103414, 'spell_ds_morchok_stomp'),
    (108571, 'spell_ds_morchok_stomp'),
    (109033, 'spell_ds_morchok_stomp'),
    (109034, 'spell_ds_morchok_stomp');

*/