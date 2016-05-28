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

/*
Encounter: Ultraxion
Dungeon: Dragon Soul
Difficulty: Normal / Heroic
Mode: 10-man / 25-man
Autor: Lazik
*/

#include "ScriptPCH.h"
#include "dragonsoul.h"
#include "TaskScheduler.h"

#define WAYPOINT_X      -1705.0f
#define WAYPOINT_Y      -2384.0f
#define WAYPOINT_Z        355.0f
#define SEARCH_RANGE      300.0f

// NPCs
enum NPC
{
    BOSS_ULTRAXION                          = 55294,
    NPC_ULTRAXION_GAUNTLET                  = 56305,
    NPC_LIGHTNING_TRIGGER                   = 119557,

    NPC_GO_GIFT_OF_LIFE_SPAWN               = 119550,
    NPC_GO_ESSENCE_OF_DREAMS_SPAWN          = 119551,
    NPC_GO_SOURCE_OF_MAGIC_SPAWN            = 119552,
};

enum Gameobjescts
{
    GO_GIFT_OF_LIFE                         = 209873,
    GO_ESSENCE_OF_DREAMS                    = 209874,
    GO_SOURCE_OF_MAGIC                      = 209875,

    GO_LESSER_CACHE_OF_ASPECTS_10N          = 210160,
    GO_LESSER_CACHE_OF_ASPECTS_25N          = 210161,
    GO_LESSER_CACHE_OF_ASPECTS_10HC         = 210162,
    GO_LESSER_CACHE_OF_ASPECTS_25HC         = 210163,
};

// Spells
enum Spells
{
    SPELL_FALL_ANIMATION                    = 94653, // Fall animation after his death
    SPELL_ULTRAXION_NORM_REALM_COSMETIC     = 105929,
    SPELL_ULTRAXION_COSMETIC                = 154, // 105211 spell - 154 misc value - AURA_REPLACEMENT_ANIM_KIT not implemented yet
    SPELL_COSMETIC_LIGHTNING_FAR            = 109405,
    SPELL_COSMETIC_LIGHTNING_CLOSE          = 109406,

    SPELL_ULTRAXION_ACHIEVEMENT_AURA        = 109188,
    SPELL_ULTRAXION_AHCIEVEMENT_FAILED      = 109194,

    SPELL_UNSTABLE_MONSTROSITY_6S           = 106372, // 6s trigger aura - 109176 / 106375
    SPELL_UNSTABLE_MONSTROSITY_5S           = 106376, // 5s trigger aura
    SPELL_UNSTABLE_MONSTROSITY_4S           = 106377, // 4s trigger aura
    SPELL_UNSTABLE_MONSTROSITY_3S           = 106378, // 3s trigger aura
    SPELL_UNSTABLE_MONSTROSITY_2S           = 106379, // 2s trigger aura
    SPELL_UNSTABLE_MONSTROSITY_1S           = 106380, // 1s trigger aura
    SPELL_UNSTABLE_MONSTROSITY_PROTECTION   = 106390, // Protects players from twilight instavility

    SPELL_TWILIGHT_INSTABILITY_DUMMY        = 106374, // Dummy effect
    SPELL_TWILIGHT_INSTABILITY_10N          = 106375, // 300 000 dmg 10N
    SPELL_TWILIGHT_INSTABILITY_25N          = 109182, // 825 000 dmg 25N
    SPELL_TWILIGHT_INSTABILITY_10HC         = 109183, // 400 000 dmg 10HC
    SPELL_TWILIGHT_INSTABILITY_25HC         = 109184, // 1 100 000 dmg 25HC

    SPELL_GROWING_INSTABILITY               = 106373,
    SPELL_MAXIMUM_INSTABILITY               = 106395,

    SPELL_TWILIGHT_SHIFT_COSMETIC_EFFECT    = 106368, // Twilight Phase - cosmetic visual
    SPELL_TWILIGHT_SHIFT                    = 106369, // Force cast 106368

    SPELL_HEROIC_WILL_ACTION_BUTTON         = 105554,
    SPELL_HEROIC_WILL                       = 106108,
    SPELL_HEROIC_WILL_COSMETIC_EFFECT       = 106175,

    SPELL_FADING_LIGHT_TANK_10N             = 105925, // from boss to player, triggered by hour of twilight, tank only
    SPELL_FADING_LIGHT_TANK_25N             = 110070,
    SPELL_FADING_LIGHT_TANK_10HC            = 110069,
    SPELL_FADING_LIGHT_TANK_25HC            = 110068,
    SPELL_FADING_LIGHT_KILL_10N             = 105926, // kill player
    SPELL_FADING_LIGHT_KILL_25N             = 110075,
    SPELL_FADING_LIGHT_KILL_10HC            = 110074,
    SPELL_FADING_LIGHT_KILL_25HC            = 110073,
    SPELL_FADING_LIGHT_DPS_10N              = 109075, // from boss, triggered by 105925, dps
    SPELL_FADING_LIGHT_DPS_25N              = 110080,
    SPELL_FADING_LIGHT_DPS_10HC             = 110079,
    SPELL_FADING_LIGHT_DPS_25HC             = 110078,
    SPELL_FADING_LIGHT_DUMMY                = 109200, // dummy

    SPELL_FADED_INTO_TWILIGHT_10N           = 105927,
    SPELL_FADED_INTO_TWILIGHT_25N           = 109461,
    SPELL_FADED_INTO_TWILIGHT_10HC          = 109462,
    SPELL_FADED_INTO_TWILIGHT_25HC          = 109463,

    SPELL_HOUR_OF_TWILIGHT_DMG              = 103327, // dmg + forse cast 109231 (Looming Darkness), force cast 106370
    SPELL_HOUR_OF_TWILIGHT_REMOVE_WILL      = 106174, // remove heroic will
    SPELL_HOUR_OF_TWILIGHT_ACHIEV_10N       = 106370, // from player, force cast achievement
    SPELL_HOUR_OF_TWILIGHT_ACHIEV_25N       = 109172,
    SPELL_HOUR_OF_TWILIGHT_ACHIEV_10HC      = 109173,
    SPELL_HOUR_OF_TWILIGHT_ACHIEV_25HC      = 109174,
    SPELL_HOUR_OF_TWILIGHT_10N              = 106371, // Eff1: 106174, Eff2: 103327, Eff3: 105925 (Fading Light)
    SPELL_HOUR_OF_TWILIGHT_25N              = 109415,
    SPELL_HOUR_OF_TWILIGHT_10HC             = 109416,
    SPELL_HOUR_OF_TWILIGHT_25HC             = 109417,
    SPELL_HOUR_OF_TWILIGHT_UNKNWON          = 106389, // Eff1: Twilight Shift, Eff2: 103327 HoT, Eff3: Dummy
    SPELL_HOUR_OF_TWILIGHT_LAUNCH           = 109323, // Some Quest start? WTF

    SPELL_TWILIGHT_BURST                    = 106415,

    SPELL_TWILIGHT_ERUPTION                 = 106388, // Kill all players

    // Heroic
    SPELL_LOOMING_DARKNESS_DUMMY            = 106498,
    SPELL_LOOMING_DARKNESS_DMG              = 109231,

    // Alexstrasza
    SPELL_GIFT_OF_LIFE_AURA                 = 105896,
    // Ysera
    SPELL_ESSENCE_OF_DREAMS_AURA            = 105900,
    SPELL_ESSENCE_OF_DREAMS_HEAL            = 105996,
    // Kalecgos
    SPELL_SOURCE_OF_MAGIC_AURA              = 105903,
    // Nozdormu
    SPELL_TIMELOOP                          = 105984,
    SPELL_TIMELOOP_HEAL                     = 105992,
    // Thrall
    SPELL_LAST_DEFENDER_OF_AZEROTH          = 106182, // scale + force cast 110327
    SPELL_LAST_DEFENDER_OF_AZEROTH_SCRIPT   = 106218,
    SPELL_LAST_DEFENDER_OF_AZEROTH_DUMMY    = 110327,
    SPELL_LAST_DEFENDER_OF_AZEROTH_WARR     = 106080,
    SPELL_LAST_DEFENDER_OF_AZEROTH_DRUID    = 106224,
    SPELL_LAST_DEFENDER_OF_AZEROTH_PALA     = 106226,
    SPELL_LAST_DEFENDER_OF_AZEROTH_DK       = 106227,
};

enum Achievements
{
    ACHIEVEMENT_MINUTES_TO_MIDNIGHT         = 6084,
    ACHIEVEMENT_HEROIC_ULTRAXION            = 6113,
};

enum Actions
{
    ACTION_TWILIGHT_ERRUPTION               = 0,
    ACTION_TWILIGHT_BURST                   = 1,
    ACTION_CAST_VISUAL_LIGHTNING            = 2,
    ACTION_CHANGE_COSMETIC_LIGHTNING        = 3,
};

enum Quotes
{
    QUOTE_AGGRO                 = 0,
    QUOTE_APPEAR                = 1,
    QUOTE_PHASE                 = 2,
    QUOTE_KILL_1                = 3,
    QUOTE_KILL_2                = 4,
    QUOTE_KILL_3                = 5,
    QUOTE_DEATH                 = 6,
    QUOTE_TWILIGHT_ERUPTION     = 7,
    QUOTE_HOUR_OF_TWILIGHT      = 8,
    QUOTE_MORE_UNSTABLE         = 9,
};

struct PlayableQuote yell[10]
{ 
    { 26314, "Now is the hour of twilight!" },
    { 26317, "I am the beginning of the end...the shadow which blots out the sun...the bell which tolls your doom..." },
    { 26318, "For this moment ALONE was I made. Look upon your death, mortals, and despair!" },
    { 26319, "Fall before Ultraxion!" },
    { 26320, "You have no hope!" },
    { 26321, "Hahahahaha!" },
    { 26316, "But...but...I am...Ul...trax...ionnnnnn..." },
    { 26315, "I WILL DRAG YOU WITH ME INTO FLAME AND DARKNESS!" },
    { 26323, "The final shred of light fades, and with it, your pitiful mortal existence!" },
    { 26325, "Through the pain and fire my hatred burns!" },
};

struct ActionAndAspect
{
    uint32 time;
    uint32 entry;
};

struct ActionAndAspect aspectAndAction [5]
{
    { 5,        NPC_THRALL },                       // 5
    { 75,       NPC_ALEXSTRASZA_THE_LIFE_BINDER },  // 80
    { 75,       NPC_YSERA_THE_AWAKENED },           // 155
    { 60,       NPC_KALECGOS },                     // 215
    { 45,       NPC_NOZDORMU_THE_TIMELESS_ONE },    // 300
};

enum Aspects
{
    THRALL                  = 0,
    ALEXSTRASZA             = 1,
    YSERA                   = 2,
    KALECGOS                = 3,
    NOZDORMU                = 4,
    MAX_ASPECTS             = 5,
};

const Position cacheSpawn = { -1753.67f, -2368.67f, 340.84f, 4.788f };

// Ultraxion
class boss_ultraxion : public CreatureScript
{
public:
    boss_ultraxion() : CreatureScript("boss_ultraxion") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_ultraxionAI(pCreature);
    }

    struct boss_ultraxionAI : public ScriptedAI
    {
        boss_ultraxionAI(Creature *creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        TaskScheduler scheduler;
        InstanceScript* instance;
        uint32 unstableMonstrosityTimer;
        uint32 unstableMonstrosityCount;
        uint32 hourOfTwilightTimer;
        uint32 twilightBurstTimer;
        uint32 currentAspect;
        bool achievement;
        bool isAboutToDie;
        bool visualLightning;

        // Shortcut for testing purpose
        void ReceiveEmote(Player* player, uint32 uiTextEmote) override
        {
            if (uiTextEmote == TEXTEMOTE_KNEEL && player->IsGameMaster())
            {
                me->AI()->DoAction(DATA_SUMMON_ULTRAXION);
            }
        }

        void Reset() override
        {
            if (instance)
            {
                if (instance->GetData(TYPE_BOSS_ULTRAXION) != DONE)
                    instance->SetData(TYPE_BOSS_ULTRAXION, NOT_STARTED);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            }

            me->SetMeleeAnimKitId(SPELL_ULTRAXION_COSMETIC);
            me->SetVisible(false);
            me->SetFlying(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);

            unstableMonstrosityTimer = 60000;
            unstableMonstrosityCount = 0;
            hourOfTwilightTimer = 45000;
            twilightBurstTimer = 4000;
            currentAspect = 0;
            achievement = true;
            isAboutToDie = false;
            visualLightning = false;

            for (uint32 goId = GO_GIFT_OF_LIFE; goId <= GO_SOURCE_OF_MAGIC; goId++)
            {
                DeleteGameObjects(goId);
            }

            // Make sure we can talk to Thrall and start encounter again
            if (Creature * pThrall = me->FindNearestCreature(NPC_THRALL, SEARCH_RANGE, true))
                pThrall->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

            scheduler.CancelAll();
            RemoveEncounterAuras();
            SpawnUltraxionGauntlet(false);

            ScriptedAI::Reset();
        }

        void EnterEvadeMode() override
        {
            for (uint32 i = THRALL; i < MAX_ASPECTS; i++)
            {
                if (Creature * pAspect = me->FindNearestCreature(aspectAndAction[i].entry, SEARCH_RANGE, true))
                    pAspect->GetAI()->DoAction(DATA_ULTRAXION_RESET);
            }
            RemoveEncounterAuras();
            SpawnUltraxionGauntlet(false);
            ScriptedAI::EnterEvadeMode();
        }

        uint32 GetData(uint32 DataId)
        {
            if (DataId == ACTION_CAST_VISUAL_LIGHTNING)
            {
                if (visualLightning == true)
                    return ACTION_CAST_VISUAL_LIGHTNING;
            }
            return 0;
        }

        void SpawnUltraxionGauntlet(bool enter = true)
        {
            if (Vehicle * veh = me->GetVehicleKit())
            {
                uint8 seats = veh->m_Seats.size();

                for (uint8 i = 0; i < seats; i++)
                {
                    if (enter)
                    {
                        if (Creature * vehiclePassenger = me->SummonCreature(NPC_ULTRAXION_GAUNTLET, 0, 0, 0))
                        {
                            vehiclePassenger->SetPhaseMask(17, true);
                            vehiclePassenger->EnterVehicle(me, i);
                        }
                    }
                    else
                    {
                        if (Unit * vehiclePassenger = veh->GetPassenger(i))
                        {
                            vehiclePassenger->ExitVehicle();
                            vehiclePassenger->ToCreature()->DespawnOrUnsummon();
                        }
                    }

                }
            }
        }

        void DoAction(const int32 action) override
        {
            switch (action)
            {
            case DATA_SUMMON_ULTRAXION:
                me->SetVisible(true);
                me->SetSpeed(MOVE_FLIGHT, 1.8f);
                me->GetMotionMaster()->MovePoint(0, WAYPOINT_X, WAYPOINT_Y, WAYPOINT_Z, false, true);

                RunPlayableQuote(yell[QUOTE_APPEAR]);

                scheduler.Schedule(Seconds(13), [this](TaskContext intro)
                {
                    if (intro.GetRepeatCounter() == 0)
                    {
                        me->SetSpeed(MOVE_FLIGHT, 1.8f);
                        // Take all players to twilight phase
                        SwitchPlayersToTwilightPhase();
                        // Make invisible all drakes and Deathwing
                        if (instance)
                            instance->SetData(DATA_ULTRAXION_DRAKES, 16);
                        RunPlayableQuote(yell[QUOTE_PHASE]);
                        SpawnUltraxionGauntlet();
                        visualLightning = true;
                        intro.Repeat(Seconds(18));
                    }
                    else
                    {
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetReactState(REACT_DEFENSIVE);
                        visualLightning = false;
                    }
                });
                break;
            case ACTION_TWILIGHT_ERRUPTION:
                me->InterruptNonMeleeSpells(false);
                me->CastSpell(me, SPELL_TWILIGHT_ERUPTION, false);
                RunPlayableQuote(yell[QUOTE_TWILIGHT_ERUPTION]);
                break;
            case ACTION_TWILIGHT_BURST:
                if (!me->HasUnitState(UNIT_STATE_CASTING))
                {
                    if (!me->IsWithinMeleeRange(me->GetVictim()))
                        me->CastSpell(me, SPELL_TWILIGHT_BURST, false);
                }
                break;
            default:
                break;
            }
        }

        void SwitchPlayersToTwilightPhase()
        {
            Map::PlayerList const &PlList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = PlList.begin(); i != PlList.end(); ++i)
            {
                if (Player* player = i->getSource())
                {
                    if (!player->IsGameMaster())
                    {
                        player->RemoveAurasDueToSpell(SPELL_HEROIC_WILL_ACTION_BUTTON);
                        player->CastSpell(player, SPELL_TWILIGHT_SHIFT_COSMETIC_EFFECT, true);
                        player->CastSpell(player, SPELL_HEROIC_WILL_ACTION_BUTTON, true);
                    }
                }
            }
        }

        void EnterCombat(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, IN_PROGRESS);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ULTRAXION_ACHIEVEMENT_AURA);
            }

            SwitchPlayersToTwilightPhase();
            RunPlayableQuote(yell[QUOTE_AGGRO]);
            me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_6S, false);

            // Aspects and their help during the encounter
            scheduler.Schedule(Seconds(aspectAndAction[currentAspect].time), [this](TaskContext aspectsHelp)
            {
                if (Creature * pAspect = me->FindNearestCreature(aspectAndAction[currentAspect].entry, SEARCH_RANGE, true))
                    pAspect->GetAI()->DoAction(DATA_HELP_AGAINST_ULTRAXION);
                currentAspect++;
                if (aspectsHelp.GetRepeatCounter() < MAX_ASPECTS)
                    aspectsHelp.Repeat(Seconds(aspectAndAction[currentAspect].time));
            });
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim && victim->GetTypeId() == TYPEID_PLAYER)
            {
                uint32 rand = urand(QUOTE_KILL_1, QUOTE_KILL_3);
                RunPlayableQuote(yell[rand]);
            }
        }

        void SummonCache(Unit * player)
        {
            uint32 cacheId = 0;
            switch (getDifficulty())
            {
                case RAID_DIFFICULTY_10MAN_NORMAL: cacheId = GO_LESSER_CACHE_OF_ASPECTS_10N;  break;
                case RAID_DIFFICULTY_25MAN_NORMAL: cacheId = GO_LESSER_CACHE_OF_ASPECTS_25N;  break;
                case RAID_DIFFICULTY_10MAN_HEROIC: cacheId = GO_LESSER_CACHE_OF_ASPECTS_10HC; break;
                case RAID_DIFFICULTY_25MAN_HEROIC: cacheId = GO_LESSER_CACHE_OF_ASPECTS_25HC; break;
                default:
                    break;
            }
            player->SummonGameObject(cacheId, cacheSpawn.GetPositionX(), cacheSpawn.GetPositionY(), cacheSpawn.GetPositionZ(), cacheSpawn.GetOrientation(), 0, 0, 0, 0, 86400);
        }

        void DamageTaken(Unit* who, uint32& damage)
        {
            if (me->GetHealth() <= damage)
            {
                damage = me->GetHealth() - 1;

                if (!isAboutToDie)
                {
                    isAboutToDie = true;
                    me->InterruptNonMeleeSpells(false);

                    for (uint32 goId = GO_GIFT_OF_LIFE; goId <= GO_SOURCE_OF_MAGIC; goId++)
                    {
                        DeleteGameObjects(goId);
                    }

                    RemoveEncounterAuras();

                    CheckForAchievementCriteria();
                    if (instance)
                    {
                        if (achievement)
                            instance->DoCompleteAchievement(ACHIEVEMENT_MINUTES_TO_MIDNIGHT);
                        if (IsHeroic())
                            instance->DoCompleteAchievement(ACHIEVEMENT_HEROIC_ULTRAXION);
                    }

                    twilightBurstTimer = 10000;
                    me->SetSpeed(MOVE_FLIGHT, 20.0f);
                    me->GetMotionMaster()->MovePoint(0, WAYPOINT_X, WAYPOINT_Y, WAYPOINT_Z - 300, false, true);
                    me->CastSpell(me, SPELL_FALL_ANIMATION, false);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                    RunPlayableQuote(yell[QUOTE_DEATH], false);
                    scheduler.CancelAll();

                    scheduler.Schedule(Seconds(4), [this, who](TaskContext /* Task context */)
                    {
                        me->Kill(me);
                        SummonCache(who);
                    });
                }
            }
        }

        void JustDied(Unit * /*who*/) override
        {
            if (instance)
            {
                instance->SetData(TYPE_BOSS_ULTRAXION, DONE);
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);

                instance->DoModifyPlayerCurrencies(CURRENCY_VALOR_POINTS, IsHeroic() ? 140 : 120, CURRENCY_SOURCE_OTHER);
                instance->DoModifyPlayerCurrencies(CURRENCY_MOTE_OF_DARKNESS, 1, CURRENCY_SOURCE_OTHER);
            }
        }

        void CheckForAchievementCriteria()
        {
            Map::PlayerList const &playerList = me->GetMap()->GetPlayers();
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                if (Player * pPlayer = i->getSource())
                {
                    if (pPlayer->HasAura(SPELL_ULTRAXION_ACHIEVEMENT_AURA))
                    {
                        if (Aura * pAura = pPlayer->GetAura(SPELL_ULTRAXION_ACHIEVEMENT_AURA))
                        {
                            if (pAura->GetStackAmount() > 1)
                                achievement = false;
                            return;
                        }
                    }
                }
            }
        }

        void CastSpellOnRandomPlayers(uint32 spellId, uint32 size, bool triggered = true, bool ignoreTanks = false, bool ignoreHealers = false)
        {
            std::list<Player*> target_list;
            target_list.clear(); // For sure
            Map * map = me->GetMap();

            if (!map)
                return;

            Map::PlayerList const& plrList = map->GetPlayers();
            if (plrList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
            {
                if (Player* pPlayer = itr->getSource())
                {
                    if (pPlayer->IsAlive() && !pPlayer->IsGameMaster())
                    {
                        if (ignoreHealers == true && ignoreTanks == true)
                        {
                            if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487) && !pPlayer->HasHealingSpec())
                                target_list.push_back(pPlayer);
                        }
                        else if (ignoreTanks == true)
                        {
                            if (!pPlayer->HasTankSpec() && !pPlayer->HasAura(5487)) // Or bear form
                                target_list.push_back(pPlayer);
                        }
                        else if (ignoreHealers == true)
                        {
                            if (!pPlayer->HasHealingSpec())
                                target_list.push_back(pPlayer);
                        } else target_list.push_back(pPlayer);
                    }
                }
            }

            for (uint32 i = 0; i < size; i++)
            {
                if (target_list.empty())
                    break;

                std::list<Player*>::iterator j = target_list.begin();
                advance(j, rand() % target_list.size()); // Pick random target

                if ((*j) && (*j)->IsInWorld())
                {
                    me->CastSpell((*j), spellId, triggered);
                    target_list.erase(j);
                }
            }
        }

        void FadingLight()
        {
            for (uint32 i = 0; i < 2; i++)
            {
                uint32 rand_time;
                rand_time = (i == 0) ? urand(10, 20) : urand(30, 35);

                scheduler.Schedule(Seconds(rand_time), [this](TaskContext /*context*/)
                {
                    int32 num_players;
                    switch (getDifficulty())
                    {
                        case RAID_DIFFICULTY_10MAN_HEROIC: num_players = 2; break;
                        case RAID_DIFFICULTY_25MAN_NORMAL: num_players = 3; break;
                        case RAID_DIFFICULTY_25MAN_HEROIC: num_players = 6; break;
                        default: /* RAID_10MAN_NORMAL */   num_players = 1; break;
                    }

                    me->CastSpell(me->GetVictim(), SPELL_FADING_LIGHT_DPS_10N, true);
                    CastSpellOnRandomPlayers(SPELL_FADING_LIGHT_DPS_10N, num_players, true, true, true);
                });
            }
        }

        void DeleteGameObjects(uint32 entry)
        {
            std::list<GameObject*> gameobjects;
            me->GetGameObjectListWithEntryInGrid(gameobjects, entry, SEARCH_RANGE);
            if (!gameobjects.empty())
            {
                for (std::list<GameObject*>::iterator itr = gameobjects.begin(); itr != gameobjects.end(); ++itr)
                {
                    (*itr)->Delete();
                }
            }
        }

        void RemoveEncounterAuras()
        {
            if (instance)
            {
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_SHIFT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TWILIGHT_SHIFT_COSMETIC_EFFECT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_HEROIC_WILL_COSMETIC_EFFECT);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_HEROIC_WILL_ACTION_BUTTON);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_HEROIC_WILL);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LOOMING_DARKNESS_DUMMY);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ULTRAXION_ACHIEVEMENT_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GIFT_OF_LIFE_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ESSENCE_OF_DREAMS_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SOURCE_OF_MAGIC_AURA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TIMELOOP);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LAST_DEFENDER_OF_AZEROTH_DK);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LAST_DEFENDER_OF_AZEROTH_PALA);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LAST_DEFENDER_OF_AZEROTH_DRUID);
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_LAST_DEFENDER_OF_AZEROTH_WARR);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            // Unstable Monstrosity ability
            if (unstableMonstrosityTimer <= diff)
            {
                unstableMonstrosityCount++;
                unstableMonstrosityTimer = 60000;

                switch (unstableMonstrosityCount)
                {
                case 1: // 6s -> 5s
                    me->RemoveAura(SPELL_UNSTABLE_MONSTROSITY_6S);
                    me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_5S, true);
                    break;
                case 2: // 5s -> 4s
                    me->RemoveAura(SPELL_UNSTABLE_MONSTROSITY_5S);
                    me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_4S, true);
                    break;
                case 3: // 4s -> 3s
                    me->RemoveAura(SPELL_UNSTABLE_MONSTROSITY_4S);
                    me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_3S, true);
                    break;
                case 4: // 3s -> 2s
                    RunPlayableQuote(yell[QUOTE_MORE_UNSTABLE]);
                    me->RemoveAura(SPELL_UNSTABLE_MONSTROSITY_3S);
                    me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_2S, true);
                    break;
                case 5: // 2s -> 1s
                    me->RemoveAura(SPELL_UNSTABLE_MONSTROSITY_2S);
                    me->CastSpell(me, SPELL_UNSTABLE_MONSTROSITY_1S, true);
                    break;
                case 6: // 1s -> Twilight Eruption
                    me->CastSpell(me, SPELL_TWILIGHT_ERUPTION, false);
                    RunPlayableQuote(yell[QUOTE_TWILIGHT_ERUPTION]);
                default:
                    break;
                }
            }
            else unstableMonstrosityTimer -= diff;

            // Hour Of Twilight
            if (hourOfTwilightTimer <= diff)
            {
                me->CastSpell(me, SPELL_HOUR_OF_TWILIGHT_10N, false);
                hourOfTwilightTimer = 45000;
                FadingLight();

                RunPlayableQuote(yell[QUOTE_HOUR_OF_TWILIGHT]);
            }
            else hourOfTwilightTimer -= diff;

            // Twilight Burst
            if (twilightBurstTimer <= diff)
            {
                me->AI()->DoAction(ACTION_TWILIGHT_BURST);
                twilightBurstTimer = 4000;
            }
            else twilightBurstTimer -= diff;

            // Melee attack
            DoMeleeAttackIfReady();
        }
    };
};

class npc_ds_ultraxion_gauntlet : public CreatureScript
{
public:
    npc_ds_ultraxion_gauntlet() : CreatureScript("npc_ds_ultraxion_gauntlet") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_ultraxion_gauntletAI(pCreature);
    }

    struct npc_ds_ultraxion_gauntletAI : public ScriptedAI
    {
        npc_ds_ultraxion_gauntletAI(Creature *creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!UpdateVictim())
                return;
        }

        void EnterEvadeMode() override { }
        void EnterCombat(Unit* /*enemy*/) override {}
        void AttackStart(Unit * who) override {}
        void MoveInLineOfSight(Unit* /*who*/) override { }
    };
};

class npc_ds_ultraxion_lightning_trigger : public CreatureScript
{
public:
    npc_ds_ultraxion_lightning_trigger() : CreatureScript("npc_ds_ultraxion_lightning_trigger") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_ds_ultraxion_lightning_triggerAI(pCreature);
    }

    struct npc_ds_ultraxion_lightning_triggerAI : public ScriptedAI
    {
        npc_ds_ultraxion_lightning_triggerAI(Creature *creature) : ScriptedAI(creature) { }

        uint32 checkIntroTimer;
        uint32 lightningCount;
        bool canLightning;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            checkIntroTimer = 1000;
            lightningCount = 0;
            canLightning = false;
        }

        void DoAction(const int32 action) override
        {
            if (action == ACTION_CAST_VISUAL_LIGHTNING)
            {
                lightningCount++;

                std::list<Creature*> lightningTrigger;
                GetCreatureListWithEntryInGrid(lightningTrigger, me, NPC_LIGHTNING_TRIGGER, SEARCH_RANGE);

                if (lightningTrigger.empty())
                    return;

                std::list<Creature*>::iterator j = lightningTrigger.begin();
                advance(j, rand() % lightningTrigger.size()); // Pick random target

                if ((*j))
                {
                    if (Creature * pUltraxion = me->FindNearestCreature(BOSS_ULTRAXION, 500.0f, true))
                    {
                        if (Vehicle * veh = pUltraxion->GetVehicleKit())
                        {
                            uint8 seats = veh->m_Seats.size();
                            uint8 randPassenger = urand(0, seats - 1);

                            if (Unit * vehiclePassenger = veh->GetPassenger(randPassenger))
                                vehiclePassenger->CastSpell((*j), (lightningCount > 15) ? SPELL_COSMETIC_LIGHTNING_FAR : SPELL_COSMETIC_LIGHTNING_CLOSE, true);
                            else
                                pUltraxion->CastSpell((*j), (lightningCount > 15) ? SPELL_COSMETIC_LIGHTNING_FAR : SPELL_COSMETIC_LIGHTNING_CLOSE, true);
                        }
                    }
                }
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!canLightning)
            {
                if (checkIntroTimer <= diff)
                {
                    if (Creature * pUltraxion = me->FindNearestCreature(BOSS_ULTRAXION, 500.0f, true))
                    {
                        if (pUltraxion->AI()->GetData(ACTION_CAST_VISUAL_LIGHTNING) == ACTION_CAST_VISUAL_LIGHTNING)
                            canLightning = true;
                    }
                    checkIntroTimer = 1000;
                }
                else checkIntroTimer -= diff;
            }
            else
            {
                if (checkIntroTimer <= diff)
                {
                    if (Creature * pUltraxion = me->FindNearestCreature(BOSS_ULTRAXION, 500.0f, true))
                    {
                        if (pUltraxion->AI()->GetData(ACTION_CAST_VISUAL_LIGHTNING) == 0)
                            canLightning = false;
                    }
                    me->AI()->DoAction(ACTION_CAST_VISUAL_LIGHTNING);
                    if (lightningCount < 5)
                        checkIntroTimer = 1000;
                    else if (lightningCount < 16)
                        checkIntroTimer = 500;
                    else
                        checkIntroTimer = 200;
                }
                else checkIntroTimer -= diff;
            }
        }
    };
};

class go_ds_ultraxion_aspect_gifts : public GameObjectScript
{
public:
    go_ds_ultraxion_aspect_gifts() : GameObjectScript("go_ds_ultraxion_aspect_gifts") { }

    bool OnGossipHello(Player* pPlayer, GameObject* pGo)
    {
        uint32 spellId = 0;
        switch (pGo->GetEntry())
        {
        case GO_GIFT_OF_LIFE:       spellId = SPELL_GIFT_OF_LIFE_AURA;       break;
        case GO_ESSENCE_OF_DREAMS:  spellId = SPELL_ESSENCE_OF_DREAMS_AURA;  break;
        case GO_SOURCE_OF_MAGIC:    spellId = SPELL_SOURCE_OF_MAGIC_AURA;    break;
        default:
            break;
        }
        pPlayer->RemoveAurasDueToSpell(SPELL_GIFT_OF_LIFE_AURA);
        pPlayer->RemoveAurasDueToSpell(SPELL_ESSENCE_OF_DREAMS_AURA);
        pPlayer->RemoveAurasDueToSpell(SPELL_SOURCE_OF_MAGIC_AURA);
        pPlayer->CastSpell(pPlayer, spellId, false);
        pGo->Delete();
        return true;
    }
};

class spell_ds_ultraxion_heroic_will : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_heroic_will() : SpellScriptLoader("spell_ds_ultraxion_heroic_will") { }

    class spell_ds_ultraxion_heroic_will_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_ultraxion_heroic_will_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (!GetCaster())
                return;

            if (Creature * pUltraxion = GetCaster()->FindNearestCreature(BOSS_ULTRAXION, SEARCH_RANGE, true))
                GetCaster()->CastSpell(pUltraxion, SPELL_ULTRAXION_NORM_REALM_COSMETIC, true);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster())
                GetCaster()->CastSpell(GetCaster(), SPELL_FADED_INTO_TWILIGHT_10N, true);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_ds_ultraxion_heroic_will_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_PACIFY_SILENCE, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_ultraxion_heroic_will_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_PACIFY_SILENCE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_ultraxion_heroic_will_AuraScript();
    }
};

class spell_ds_ultraxion_fading_light : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_fading_light() : SpellScriptLoader("spell_ds_ultraxion_fading_light") { }

    class spell_ds_ultraxion_fading_light_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_ultraxion_fading_light_AuraScript);

        void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (!GetCaster())
                return;

            Aura * aura = aurEff->GetBase();

            uint32 duration = urand((GetCaster()->GetMap()->IsHeroic() ? 3000 : 5000), 9000);
            aura->SetDuration(duration);
            aura->SetMaxDuration(duration);
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit * target = GetTarget();
            if (!target)
                return;

            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
            {
                if (target->HasAura(SPELL_TWILIGHT_SHIFT_COSMETIC_EFFECT))
                    target->CastSpell(target, SPELL_FADING_LIGHT_KILL_10N, true);
                else if (target->HasAura(SPELL_HEROIC_WILL_COSMETIC_EFFECT))
                {
                    target->CastSpell(target, SPELL_FADED_INTO_TWILIGHT_10N, true);
                    target->CastSpell(target, SPELL_TWILIGHT_SHIFT_COSMETIC_EFFECT, true);
                }
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_ds_ultraxion_fading_light_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_ds_ultraxion_fading_light_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_ultraxion_fading_light_AuraScript();
    }
};

class HasHeroicWill
{
public:
    bool operator()(WorldObject* object) const
    {
        if (Player * pl = object->ToPlayer())
        {
            if (pl->HasAura(SPELL_HEROIC_WILL_COSMETIC_EFFECT))
                return true;
        }
        return false;
    }
};

class spell_ds_ultraxion_twilight_instability : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_twilight_instability() : SpellScriptLoader("spell_ds_ultraxion_twilight_instability") {}

    class spell_ds_ultraxion_twilight_instability_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ultraxion_twilight_instability_SpellScript);

        uint32 targetsCount = 0;

        void FilterTargets(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(HasHeroicWill());

            targetsCount = targets.size();
        }

        void HandleScript()
        {
            Unit * caster = GetCaster();
            Unit * target = GetHitUnit();

            if (!caster || !target)
                return;

            int bp0 = 0;

            switch (caster->GetMap()->GetDifficulty())
            {
                case RAID_DIFFICULTY_10MAN_HEROIC: bp0 = 400000;  break;
                case RAID_DIFFICULTY_25MAN_NORMAL: bp0 = 825000;  break;
                case RAID_DIFFICULTY_25MAN_HEROIC: bp0 = 1100000; break;
                default:  /*RAID_10MAN_NORMAL*/    bp0 = 300000;  break;
            }

            bp0 /= targetsCount;
            if (!caster->HasUnitState(UNIT_STATE_CASTING)) // Prevent during Hour of Twilight
            {
                if (Vehicle * veh = caster->GetVehicleKit())
                {
                    uint8 seats = veh->m_Seats.size();
                    uint8 randPassenger = urand(0, seats);

                    if (Unit * vehiclePassenger = veh->GetPassenger(randPassenger))
                        vehiclePassenger->CastCustomSpell(target, SPELL_TWILIGHT_INSTABILITY_10N, &bp0, 0, 0, true);
                    else
                        caster->CastCustomSpell(target, SPELL_TWILIGHT_INSTABILITY_10N, &bp0, 0, 0, true);
                }
            }
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_twilight_instability_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENEMY_SRC);
            OnHit += SpellHitFn(spell_ds_ultraxion_twilight_instability_SpellScript::HandleScript);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ultraxion_twilight_instability_SpellScript();
    }
};

class spell_ds_ultraxion_hour_of_twilight_dmg : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_hour_of_twilight_dmg() : SpellScriptLoader("spell_ds_ultraxion_hour_of_twilight_dmg") { }

    class spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript);

        void FilterTargetsDamage(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(HasHeroicWill());

            uint32 min_players = 1;
            
            switch (GetCaster()->GetMap()->GetDifficulty())
            {
                case RAID_DIFFICULTY_10MAN_HEROIC: min_players = 2; break;
                case RAID_DIFFICULTY_25MAN_NORMAL: min_players = 3; break;
                case RAID_DIFFICULTY_25MAN_HEROIC: min_players = 5; break;
                default:/*RAID_DIFFICULTY_10MAN N*/min_players = 1; break;
            }

            if (targets.size() < min_players)
            {
                if (Creature* pUltraxion = GetCaster()->ToCreature())
                    pUltraxion->AI()->DoAction(ACTION_TWILIGHT_ERRUPTION);
            }
            GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_MONSTROSITY_PROTECTION, true);
        }

        void FilterTargetsDarknessAndAchievement(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(HasHeroicWill());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsDamage, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsDarknessAndAchievement, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript::FilterTargetsDarknessAndAchievement, EFFECT_2, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ultraxion_hour_of_twilight_dmg_SpellScript();
    }
};

class spell_ds_ultraxion_time_loop : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_time_loop() : SpellScriptLoader("spell_ds_ultraxion_time_loop") { }

    class spell_ds_ultraxion_time_loop_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ds_ultraxion_time_loop_AuraScript);

        bool Load()
        {
            return true;
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            amount = -1;
        }

        void Absorb(AuraEffect * aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * victim = GetTarget();

            if (dmgInfo.GetSpellInfo() == nullptr)
                return;

            if (dmgInfo.GetDamage() >= victim->GetHealth() && dmgInfo.GetSpellInfo()->Id != SPELL_TWILIGHT_ERUPTION)
            {
                int32 healAmount = victim->GetMaxHealth();
                victim->CastCustomSpell(victim, SPELL_TIMELOOP_HEAL, &healAmount, nullptr, nullptr, true);
                absorbAmount = dmgInfo.GetDamage() + 1;
                aurEff->GetBase()->Remove();
            }
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_ds_ultraxion_time_loop_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_ds_ultraxion_time_loop_AuraScript::Absorb, EFFECT_1);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_ds_ultraxion_time_loop_AuraScript();
    }
};

class spell_ds_ultraxion_essence_of_dreams : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_essence_of_dreams() : SpellScriptLoader("spell_ds_ultraxion_essence_of_dreams") {}

    class spell_ds_ultraxion_essence_of_dreams_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ultraxion_essence_of_dreams_SpellScript);

        uint32 targetsCount = 0;

        void TargetsCount(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targetsCount = targets.size();
        }

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            if (!GetCaster() || !GetHitUnit())
                return;

            if (targetsCount == 0)
                return;

            float heal = 0.0f;
            heal = GetHitHeal() / targetsCount;

            SetHitHeal((int32)heal);
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_essence_of_dreams_SpellScript::TargetsCount, EFFECT_0, TARGET_UNIT_AREA_ENTRY_DST);
            OnEffect += SpellEffectFn(spell_ds_ultraxion_essence_of_dreams_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ultraxion_essence_of_dreams_SpellScript();
    }
};

class IsNotTank
{
public:
    bool operator()(WorldObject* object) const
    {
        if (Player * pl = object->ToPlayer())
        {
            switch (pl->GetActiveTalentBranchSpec())
            {
            case SPEC_WARRIOR_PROTECTION:
            case SPEC_PALADIN_PROTECTION:
            case SPEC_DK_BLOOD:
                return false;
            case SPEC_DRUID_FERAL:
                if (pl->HasAura(5487)) // Bear Form
                    return false;
                else
                    return true;
            default:
                return true;
            }
            return true;
        }
        return true;
    }
};

class spell_ds_ultraxion_last_defender_of_azeroth : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_last_defender_of_azeroth() : SpellScriptLoader("spell_ds_ultraxion_last_defender_of_azeroth") { }

    class spell_ds_ultraxion_last_defender_of_azeroth_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ultraxion_last_defender_of_azeroth_SpellScript);

        void FilterTargets(std::list<Unit*>& targets)
        {
            if (!GetCaster())
                return;

            targets.remove_if(IsNotTank());
        }

        void Register()
        {
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_last_defender_of_azeroth_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_AREA_ENTRY_SRC);
            OnUnitTargetSelect += SpellUnitTargetFn(spell_ds_ultraxion_last_defender_of_azeroth_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_AREA_ENTRY_SRC);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ultraxion_last_defender_of_azeroth_SpellScript();
    }
};

class spell_ds_ultraxion_last_defender_of_azeroth_dummy : public SpellScriptLoader
{
public:
    spell_ds_ultraxion_last_defender_of_azeroth_dummy() : SpellScriptLoader("spell_ds_ultraxion_last_defender_of_azeroth_dummy") { }

    class spell_ds_ultraxion_last_defender_of_azeroth_dummy_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ds_ultraxion_last_defender_of_azeroth_dummy_SpellScript);

        void HandleDummy()
        {
            Player * victim = GetCaster()->ToPlayer();
            if (!GetCaster() || !victim)
                return;

            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            switch (victim->ToPlayer()->GetActiveTalentBranchSpec())
            {
            case SPEC_WARRIOR_PROTECTION:
                victim->CastSpell(victim, SPELL_LAST_DEFENDER_OF_AZEROTH_WARR, true);
                break;
            case SPEC_PALADIN_PROTECTION:
                victim->CastSpell(victim, SPELL_LAST_DEFENDER_OF_AZEROTH_PALA, true);
                break;
            case SPEC_DK_BLOOD:
                victim->CastSpell(victim, SPELL_LAST_DEFENDER_OF_AZEROTH_DK, true);
                break;
            case SPEC_DRUID_FERAL:
                victim->CastSpell(victim, SPELL_LAST_DEFENDER_OF_AZEROTH_DRUID, true);
                break;
            default:
                break;
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_ds_ultraxion_last_defender_of_azeroth_dummy_SpellScript::HandleDummy);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_ds_ultraxion_last_defender_of_azeroth_dummy_SpellScript();
    }
};

void AddSC_boss_ultraxion()
{
    // Boss
    new boss_ultraxion();                                       // 55294
    new npc_ds_ultraxion_gauntlet();                            // 56305
    new npc_ds_ultraxion_lightning_trigger();                   // 119558

    // Spells
    new spell_ds_ultraxion_heroic_will();                       // 106108
    new spell_ds_ultraxion_fading_light();                      // 109075, 110078, 110079, 110080
    new spell_ds_ultraxion_twilight_instability();              // 109176
    new spell_ds_ultraxion_hour_of_twilight_dmg();              // 103327
    new spell_ds_ultraxion_time_loop();                         // 105984
    new spell_ds_ultraxion_essence_of_dreams();                 // 105996
    new spell_ds_ultraxion_last_defender_of_azeroth();          // 106182
    new spell_ds_ultraxion_last_defender_of_azeroth_dummy();    // 110327

    // Gameobjects
    new go_ds_ultraxion_aspect_gifts();                         // 209873, 209874, 209875
}

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////         SQL QUERY          ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
/*
    --NPC
    --Ultraxion
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
    `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
    `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
    `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
    `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('55294','0','0','0','0','0','39099','0','0','0','Ultraxion','','','0','88','88','3','14','14','0','1.42857','1','1','1','0',
    '0','0','0','1','2000','2000','1','32832','4','0','0','0','0','0','0','0','0','2','2097260','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','1847','0','0','','0','3','660','0.01056','1','0','0','0','0','0','0','0','144','1','0','0','1','boss_ultraxion','15595');

    --Ultraxion Gauntlet
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
    `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
    `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
    `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
    `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('56305','0','0','0','0','0','24103','38497','0','0','Ultraxion Gauntlet','','','0','87','87','3','14','14','0','1.14286','1','1','1','0',
    '0','0','0','1','2000','2000','1','0','0','0','0','0','0','0','0','0','0','11','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','7','100','1','1','0','0','0','0','0','0','0','0','1','0','0','0','npc_ds_ultraxion_gauntlet','15595');

    --Ultraxion Lightning Main
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`,
    `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
    `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
    `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
    `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119558','0','0','0','0','0','11686','0','0','0','Ultraxion Lightning Triger Main',NULL,NULL,'0','85','85','3','35','35','0','1','1.14286','1','0','0','0',
    '0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','128','npc_ds_ultraxion_lightning_trigger','1');

    --Ultraxion Lightning Adds
    REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`,
    `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`,
    `trainer_spell`,`trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`,
    `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`,
    `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) values('119557','0','0','0','0','0','11686','0','0','0','Ultraxion Lightning Trigger Adds',NULL,NULL,'0','85','85','3','35','35','0','1','1.14286','1','0','0','0',
    '0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','0','1','0','0','128','','1');

    --SPAWN
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('15430850','55294','967','15','17','0','0','-1386.04','-2385.98','142.901','3.08266','10080','0','0','1','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219205','119557','967','15','16','0','0','-1734.89','-2400.66','340.855','0.0588808','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219206','119557','967','15','16','0','0','-1736.93','-2376.16','340.856','0.0981507','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219207','119557','967','15','16','0','0','-1730.43','-2383.65','340.858','0.0981507','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219208','119557','967','15','16','0','0','-1729.63','-2391.79','340.859','0.0981507','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219209','119557','967','15','16','0','0','-1736.95','-2395.49','340.849','0.117777','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219210','119557','967','15','16','0','0','-1738.66','-2380.53','340.849','0.0627987','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219211','119557','967','15','16','0','0','-1745.85','-2378.25','340.844','0.0627987','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219212','119557','967','15','16','0','0','-1746.29','-2371.18','340.853','0.0627987','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219213','119557','967','15','16','0','0','-1744.16','-2399.82','340.842','0.0824336','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219214','119557','967','15','16','0','0','-1743.68','-2405.58','340.847','0.0824336','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219215','119557','967','15','16','0','0','-1750.94','-2402.19','340.834','0.0824336','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219216','119557','967','15','16','0','0','-1753.12','-2375.82','340.837','0.0824336','300','0','0','77491','0','0','0','0','0','0');
    INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `modelid`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`, `curhealth`, `curmana`, `DeathState`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`) values('219204','119558','967','15','16','0','0','-1742.94','-2388.69','340.838','0.0942256','300','0','0','77491','0','0','0','0','0','0');

    --GAMEOBJECTS
    REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`,
    `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) values('209873','10','8513','Gift of Life','','','','0','0','6','0','0','0','0','0','0','86','0','0','3000','0','1','0','0','0','0','105896','0','0','0','0','0','0','0','0','0','0','0','13132','1','0','0','0','0','0','0','0','0','','go_ds_ultraxion_aspect_gifts','15595');
    REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`,
    `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) values('209874','10','7822','Essence of Dreams','','','','0','0','3','0','0','0','0','0','0','86','0','0','3000','0','1','0','0','0','0','105900','0','0','0','0','0','0','0','0','0','0','0','13132','1','0','0','0','0','0','0','0','0','','go_ds_ultraxion_aspect_gifts','15595');
    REPLACE INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `faction`, `flags`, `size`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `data0`, `data1`, `data2`, `data3`, `data4`, `data5`, `data6`, `data7`, `data8`, `data9`, `data10`, `data11`, `data12`, `data13`, `data14`, `data15`, `data16`, `data17`, `data18`, `data19`, `data20`,
    `data21`, `data22`, `data23`, `data24`, `data25`, `data26`, `data27`, `data28`, `data29`, `data30`, `data31`, `AIName`, `ScriptName`, `WDBVerified`) values('209875','10','7841','Source of Magic','','','','0','0','3','0','0','0','0','0','0','86','0','0','3000','0','1','0','0','0','0','105903','0','0','0','0','0','0','0','0','0','0','0','13132','1','0','0','0','0','0','0','0','0','','go_ds_ultraxion_aspect_gifts','15595');

    --SPELL SCRIPTS
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('109176','spell_ds_ultraxion_twilight_instability');
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('103327','spell_ds_ultraxion_hour_of_twilight_dmg');
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('106108','spell_ds_ultraxion_heroic_will');

    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('105984','spell_ds_ultraxion_time_loop');
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('105996','spell_ds_ultraxion_essence_of_dreams');
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('106182','spell_ds_ultraxion_last_defender_of_azeroth');
    insert into `spell_script_names` (`spell_id`, `ScriptName`) values('110327','spell_ds_ultraxion_last_defender_of_azeroth_dummy');

    insert into `spell_script_names` (`spell_id`, `ScriptName`) values
        ('109075','spell_ds_ultraxion_fading_light'),
        ('110078','spell_ds_ultraxion_fading_light'),
        ('110079','spell_ds_ultraxion_fading_light'),
        ('110080','spell_ds_ultraxion_fading_light');
*/