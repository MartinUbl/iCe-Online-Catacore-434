/**
 * Darkmoon island scripts
 *
 * Copyright (c) iCe Online, 2006-2013
 *
 **/

enum DarkmoonQuests
{
    QUEST_HUMANOID_CANNONBALL   = 29436,
    QUEST_TARGET_TURTLE         = 29455,
    QUEST_HE_SHOOTS_HE_SCORES   = 29438,
    QUEST_ITS_HAMMER_TIME       = 29463,
    QUEST_TONK_COMMANDER        = 29434,
};

#define ITEM_DARKMOON_GAME_TOKEN 71083

const Position cannonPosition = {-4021.4734f, 6300.0625f, 18.63f, 3.31187f};
const Position cannonTargetPosition = { -4478.057f, 6221.55615f, -1.5959f, 0.0f};

#define CANNON_TARGET_MAX_RADIUS 7.0f

// start 1s-60s, next 30s-180s
#define MAXIMA_QUOTE_1 "Cannon blast! Who wants to get shot out of a cannon?"
#define MAXIMA_QUOTE_2 "Don't worry about your weight, this cannon can handle any payload!"
#define MAXIMA_QUOTE_3 "Step up to get blown up!"
#define MAXIMA_QUOTE_4 "Transportation to the other end of the faire!"

#define MAXIMA_QUOTES_TOTAL 4
const char* maximaQuotes[MAXIMA_QUOTES_TOTAL] = {
    MAXIMA_QUOTE_1, MAXIMA_QUOTE_2, MAXIMA_QUOTE_3, MAXIMA_QUOTE_4
};

#define MAXIMA_TEXT_ID_1 120001
#define MAXIMA_GOSSIP_1_INFO "How do I use the cannon?"
#define MAXIMA_GOSSIP_1_LAUNCH "Launch me! |cFF0000FF(Darkmoon Game Token)|r"
#define MAXIMA_TEXT_ID_2 120002
#define MAXIMA_GOSSIP_2_UNDERSTAND "I understand"

enum CannonballSpells
{
    SPELL_CANNONBALL_INVISIBILITY  = 102112,
    SPELL_CANNONBALL_LAUNCH_VISUAL = 102115,
    SPELL_CANNONBALL_ACTIONBAR     = 102121,
    SPELL_CANNONBALL_WINGS         = 102116,
    SPELL_CANNONBALL_KILL_CREDIT   = 100962,
    SPELL_CANNONBALL_BULLSEYE      = 62173,
    SPELL_STUN_SELF                = 94563,
    SPELL_HELPER_FLY               = 34873,
};

#define ACHIEVEMENT_BLASTENHEIMER_BULLSEYE 6021

class npc_maxima_darkmoon: public CreatureScript
{
    public:
        npc_maxima_darkmoon(): CreatureScript("npc_maxima_darkmoon")
        {
        }

        struct npc_maxima_darkmoonAI: public ScriptedAI
        {
            npc_maxima_darkmoonAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 nextQuoteTimer;
            uint8 nextQuote;

            void Reset()
            {
                nextQuoteTimer = urand(1, 60)*1000;
                nextQuote = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextQuoteTimer <= diff)
                {
                    nextQuoteTimer = 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(maximaQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= MAXIMA_QUOTES_TOTAL)
                        nextQuote = 0;
                }
                else
                    nextQuoteTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_maxima_darkmoonAI(c);
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MAXIMA_GOSSIP_1_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MAXIMA_GOSSIP_1_LAUNCH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(MAXIMA_TEXT_ID_1, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Info
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MAXIMA_GOSSIP_2_UNDERSTAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->SEND_GOSSIP_MENU(MAXIMA_TEXT_ID_2, pCreature->GetGUID());
                return true;
            }
            // Launch
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                if (pPlayer->GetItemCount(ITEM_DARKMOON_GAME_TOKEN, false) > 0)
                {
                    pPlayer->DestroyItemCount(ITEM_DARKMOON_GAME_TOKEN, 1, true);
                    pPlayer->NearTeleportTo(cannonPosition.GetPositionX(), cannonPosition.GetPositionY(), cannonPosition.GetPositionZ(), cannonPosition.GetOrientation(), false);
                    if (Aura* flyAura = pPlayer->AddAura(SPELL_HELPER_FLY, pPlayer))
                        flyAura->SetDuration(5000);
                    pPlayer->CastSpell(pPlayer, SPELL_CANNONBALL_INVISIBILITY, true);
                    pPlayer->CastSpell(pPlayer, SPELL_STUN_SELF, true);
                }
                else
                {
                    pPlayer->GetSession()->SendNotification("You don't have enough Darkmoon Game Tokens!");
                }
                return true;
            }
            // "I understand"
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                return OnGossipHello(pPlayer, pCreature);
            }

            return false;
        }
};

class npc_cannonball_bunny: public CreatureScript
{
    public:
        npc_cannonball_bunny(): CreatureScript("npc_cannonball_bunny")
        {
        }

        struct npc_cannonball_bunnyAI: public ScriptedAI
        {
            npc_cannonball_bunnyAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 nextCheckTimer;

            void Reset()
            {
                nextCheckTimer = 1000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextCheckTimer <= diff)
                {
                    nextCheckTimer = 1000;

                    Map::PlayerList const& plList = me->GetMap()->GetPlayers();
                    Player* tmp;
                    float dist;
                    uint32 creditcount;
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    {
                        tmp = itr->getSource();
                        if (!tmp)
                            continue;

                        if (!tmp->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
                            continue;

                        if (!tmp->HasAura(SPELL_CANNONBALL_ACTIONBAR))
                            continue;

                        tmp->RemoveAurasDueToSpell(SPELL_CANNONBALL_ACTIONBAR);

                        dist = tmp->GetDistance2d(cannonTargetPosition.GetPositionX(), cannonTargetPosition.GetPositionY());
                        if (dist > CANNON_TARGET_MAX_RADIUS)
                            continue;

                        creditcount = 5 - (uint32)ceil(5.0f*(dist / CANNON_TARGET_MAX_RADIUS));

                        if (creditcount == 0 || creditcount > 5)
                            continue;

                        for (uint32 i = 0; i < creditcount; i++)
                            tmp->CastSpell(tmp, SPELL_CANNONBALL_KILL_CREDIT, true);

                        // Achievement: Blastenheimer Bullseye
                        if (creditcount == 5)
                        {
                            tmp->CastSpell(tmp, SPELL_CANNONBALL_BULLSEYE, true);
                            AchievementEntry const* achiev = sAchievementStore.LookupEntry(ACHIEVEMENT_BLASTENHEIMER_BULLSEYE);
                            tmp->CompletedAchievement(achiev);
                        }
                    }
                }
                else
                    nextCheckTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_cannonball_bunnyAI(c);
        }
};

class spell_cannon_prep: public SpellScriptLoader
{
    public:
        spell_cannon_prep(): SpellScriptLoader("spell_cannon_prep")
        {
        }

        class spell_cannon_prep_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_cannon_prep_AuraScript);

            enum Spels
            {
                SPELL_CANNON_PREP = 102112,
            };

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_CANNON_PREP);
            }

            bool Load()
            {
                return GetUnitOwner()->ToPlayer();
            }

            void EffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner())
                    return;

                Player* caster = GetUnitOwner()->ToPlayer();

                if (!caster)
                     return;

                caster->RemoveAurasDueToSpell(SPELL_HELPER_FLY);
                caster->RemoveAurasDueToSpell(SPELL_STUN_SELF);
                caster->CastSpell(caster, SPELL_CANNONBALL_ACTIONBAR, true);
                caster->CastSpell(caster, SPELL_CANNONBALL_LAUNCH_VISUAL, true);
                caster->CastSpell(caster, SPELL_CANNONBALL_WINGS, true);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_cannon_prep_AuraScript::EffectRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_cannon_prep_AuraScript();
        }
};

#define FOZLEBUB_TEXT_ID_1 120007
#define FOZLEBUB_GOSSIP_TELEPORT "Teleport me to the cannon |cFF0000FF(30 silver)|r"
#define FOZLEBUB_GOSSIP_CANCEL "I don't need a teleport"

#define FOZLEBUB_TELEPORT_COST 30*SILVER
#define SPELL_CANNONBALL_TELEPORT_BACK 109244

class npc_teleporter_fozlebub: public CreatureScript
{
    public:
        npc_teleporter_fozlebub(): CreatureScript("npc_teleporter_fozlebub")
        {
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, FOZLEBUB_GOSSIP_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, FOZLEBUB_GOSSIP_CANCEL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(FOZLEBUB_TEXT_ID_1, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Teleport
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                if (pPlayer->HasEnoughMoney(FOZLEBUB_TELEPORT_COST))
                {
                    pPlayer->ModifyMoney(-FOZLEBUB_TELEPORT_COST);
                    pCreature->CastSpell(pPlayer, SPELL_CANNONBALL_TELEPORT_BACK, true);
                }
                else
                    pPlayer->GetSession()->SendNotification("You don't have enough money!");
                return true;
            }
            // Cancel
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                // menu already closed
                return true;
            }

            return false;
        }
};

/*

SQL

UPDATE creature_template SET ScriptName = 'npc_maxima_darkmoon', AIName = '' WHERE entry = 15303;
UPDATE creature_template SET ScriptName = 'npc_cannonball_bunny', AIName = '' WHERE entry = 33068;
DELETE FROM spell_script_names WHERE spell_id = 102112;
INSERT INTO spell_script_names VALUES (102112, 'spell_cannon_prep');
UPDATE creature_template SET ScriptName = 'npc_teleporter_fozlebub', AIName = '' WHERE entry = 57850;
INSERT INTO spell_target_position VALUES (109244, 974, -4017.340820, 6284.694336, 12.8, 1.2);

*/

/*
 * Quest: Target: Turtle
 */

#define JESSICA_QUOTE_1 "Hey, hey! Think you can land a ring on a slow moving turtle?"
#define JESSICA_QUOTE_2 "Simple game, for simple folk. Think you can manage it?"
#define JESSICA_QUOTE_3 "Toss a ring, win a prize!"
#define JESSICA_QUOTE_4 "You look like you've got quite the arm there. Care to give this game a try?"

#define JESSICA_QUOTES_TOTAL 4
const char* jessicaQuotes[JESSICA_QUOTES_TOTAL] = {
    JESSICA_QUOTE_1, JESSICA_QUOTE_2, JESSICA_QUOTE_3, JESSICA_QUOTE_4
};

#define JESSICA_TEXT_ID_1 120003
#define JESSICA_GOSSIP_1_INFO "How do I play the Ring Toss?"
#define JESSICA_GOSSIP_1_LAUNCH "Ready to play! |cFF0000FF(Darkmoon Game Token)|r"
#define JESSICA_TEXT_ID_2 120004
#define JESSICA_GOSSIP_2_UNDERSTAND "I understand"

enum RingTossSpells
{
    SPELL_RINGTOSS_ENABLE   = 102058,
    SPELL_RINGTOSS_TOSS     = 101695,
    SPELL_RINGTOSS_HIT      = 101699,
    SPELL_RINGTOSS_TURTLE_CIRCLE_1 = 101734,
    SPELL_RINGTOSS_TURTLE_CIRCLE_2 = 101736,
    SPELL_RINGTOSS_TURTLE_CIRCLE_3 = 101738,
    SPELL_RINGTOSS_KILL_CREDIT = 101807,
};

#define NPC_TURTLE_RINGTOSS 54490

class npc_jessica_darkmoon: public CreatureScript
{
    public:
        npc_jessica_darkmoon(): CreatureScript("npc_jessica_darkmoon")
        {
        }

        struct npc_jessica_darkmoonAI: public ScriptedAI
        {
            npc_jessica_darkmoonAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 nextQuoteTimer;
            uint8 nextQuote;

            void Reset()
            {
                nextQuoteTimer = urand(1, 60)*1000;
                nextQuote = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextQuoteTimer <= diff)
                {
                    nextQuoteTimer = 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(jessicaQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= JESSICA_QUOTES_TOTAL)
                        nextQuote = 0;
                }
                else
                    nextQuoteTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_jessica_darkmoonAI(c);
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JESSICA_GOSSIP_1_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JESSICA_GOSSIP_1_LAUNCH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(JESSICA_TEXT_ID_1, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Info
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, JESSICA_GOSSIP_2_UNDERSTAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->SEND_GOSSIP_MENU(JESSICA_TEXT_ID_2, pCreature->GetGUID());
                return true;
            }
            // Ready to play
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                if (pPlayer->GetItemCount(ITEM_DARKMOON_GAME_TOKEN, false) > 0)
                {
                    pPlayer->DestroyItemCount(ITEM_DARKMOON_GAME_TOKEN, 1, true);

                    pPlayer->CastSpell(pPlayer, SPELL_RINGTOSS_ENABLE, true);
                }
                else
                {
                    pPlayer->GetSession()->SendNotification("You don't have enough Darkmoon Game Tokens!");
                }
                return true;
            }
            // "I understand"
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                return OnGossipHello(pPlayer, pCreature);
            }

            return false;
        }
};

class spell_ring_toss_enable: public SpellScriptLoader
{
    public:
        spell_ring_toss_enable(): SpellScriptLoader("spell_ring_toss_enable")
        {
        }

        class spell_ring_toss_enable_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ring_toss_enable_AuraScript);

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_RINGTOSS_ENABLE);
            }

            bool Load()
            {
                return GetUnitOwner()->ToPlayer();
            }

            void EffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner())
                    return;

                Player* caster = GetUnitOwner()->ToPlayer();

                if (!caster)
                     return;

                caster->SetMaxPower(POWER_SCRIPTED, 10);
                caster->SetPower(POWER_SCRIPTED, 10);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_ring_toss_enable_AuraScript::EffectApply, EFFECT_0, SPELL_AURA_OVERRIDE_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_ring_toss_enable_AuraScript();
        }
};

class spell_ring_toss_trigger: public SpellScriptLoader
{
    public:
        spell_ring_toss_trigger(): SpellScriptLoader("spell_ring_toss_trigger") { }

        class spell_ring_toss_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ring_toss_trigger_SpellScript);

            void HitHandler()
            {
                if (!GetCaster())
                    return;

                if (GetCaster()->GetPower(POWER_SCRIPTED) == 0)
                    GetCaster()->RemoveAurasDueToSpell(SPELL_RINGTOSS_ENABLE);

                WorldLocation* loc = GetTargetDest();

                if (loc)
                    GetCaster()->CastSpell(loc->GetPositionX(), loc->GetPositionY(), loc->GetPositionZ(), SPELL_RINGTOSS_HIT, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_ring_toss_trigger_SpellScript::HitHandler);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ring_toss_trigger_SpellScript();
        }
};

class spell_ring_toss_hit: public SpellScriptLoader
{
    public:
        spell_ring_toss_hit(): SpellScriptLoader("spell_ring_toss_hit") { }

        class spell_ring_toss_hit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ring_toss_hit_SpellScript);

            void HitHandler()
            {
                if (!GetCaster())
                    return;

                WorldLocation* loc = GetTargetDest();

                if (loc)
                {
                    Creature* turtle = GetClosestCreatureWithEntry(GetCaster(), NPC_TURTLE_RINGTOSS, 35.0f, true);
                    if (turtle && turtle->IsWithinDist2d(loc->GetPositionX(), loc->GetPositionY(), 1.2f))
                    {
                        if (turtle->HasAura(SPELL_RINGTOSS_TURTLE_CIRCLE_1))
                        {
                            if (turtle->HasAura(SPELL_RINGTOSS_TURTLE_CIRCLE_2))
                                turtle->CastSpell(turtle, SPELL_RINGTOSS_TURTLE_CIRCLE_3, true);
                            else
                                turtle->CastSpell(turtle, SPELL_RINGTOSS_TURTLE_CIRCLE_2, true);
                        }
                        else
                            turtle->CastSpell(turtle, SPELL_RINGTOSS_TURTLE_CIRCLE_1, true);

                        return;
                    }
                }

                PreventHitEffect(EFFECT_0);
            }

            void AfterHitHandler()
            {
                if (GetCaster())
                {
                    Player* pl = GetCaster()->ToPlayer();
                    if (pl)
                    {
                        if (pl->GetReqKillOrCastCurrentCount(QUEST_TARGET_TURTLE, 54495) >= 3)
                            pl->RemoveAurasDueToSpell(SPELL_RINGTOSS_ENABLE);
                    }
                }
            }

            void Register()
            {
                BeforeHit += SpellHitFn(spell_ring_toss_hit_SpellScript::HitHandler);
                AfterHit += SpellHitFn(spell_ring_toss_hit_SpellScript::AfterHitHandler);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ring_toss_hit_SpellScript();
        }
};

/*

SQL

UPDATE creature_template SET ScriptName = 'npc_jessica_darkmoon', AIName = '' WHERE entry = 54485;
REPLACE INTO conditions VALUES (13, 0, 101695, 0, 18, 1, 54490, 0, 0, '', 'Ring Toss - implicit target turtle');
REPLACE INTO conditions VALUES (13, 0, 101699, 0, 18, 1, 54490, 0, 0, '', 'Ring Toss - implicit target turtle');
DELETE FROM spell_script_names WHERE spell_id IN (102058, 101695, 101699, 101807);
INSERT INTO spell_script_names VALUES (102058, 'spell_ring_toss_enable'), (101695, 'spell_ring_toss_trigger'), (101807, 'spell_ring_toss_hit');

*/

/*
 * Quest: He Shoots, He Scores!
 */

#define RINLING_QUOTE_1 "Come play a game so simple that even you can do it!"
#define RINLING_QUOTE_2 "Guns, guns, guns! C'mon, pal!"
#define RINLING_QUOTE_3 "If you can shoot, you can score some sweet prizes!"
#define RINLING_QUOTE_4 "Shoot for loot! Who wants to shoot for some loot?"
#define RINLING_QUOTE_5 "Step right up and take your best shot!"
#define RINLING_QUOTE_6 "Test your aim! Test your aim!"
#define RINLING_QUOTE_7 "Test your skill, win a prize!"

#define RINLING_QUOTES_TOTAL 7
const char* rinlingQuotes[RINLING_QUOTES_TOTAL] = {
    RINLING_QUOTE_1, RINLING_QUOTE_2, RINLING_QUOTE_3, RINLING_QUOTE_4, RINLING_QUOTE_5, RINLING_QUOTE_6, RINLING_QUOTE_7
};

#define RINLING_TEXT_ID_1 120005
#define RINLING_GOSSIP_1_INFO "How does the Shooting Gallery work?"
#define RINLING_GOSSIP_1_START "Let's shoot! |cFF0000FF(Darkmoon Game Token)|r"
#define RINLING_TEXT_ID_2 120006
#define RINLING_GOSSIP_2_UNDERSTAND "I understand"

enum HeShootsSpells
{
    SPELL_HESHOOTS_CRACKSHOT_ENABLE         = 101871,
    SPELL_HESHOOTS_SHOOT                    = 101872,
    SPELL_HESHOOTS_TARGET_INDICATOR         = 101010,
    SPELL_HESHOOTS_TARGET_INDICATOR_VISUAL  = 43313,
    SPELL_HESHOOTS_KILL_CREDIT              = 43300,
    SPELL_HESHOOTS_KILL_CREDIT_BONUS        = 101012,
};

#define ACHIEVEMENT_QUICK_SHOT 6022

class npc_rinling_darkmoon: public CreatureScript
{
    public:
        npc_rinling_darkmoon(): CreatureScript("npc_rinling_darkmoon")
        {
        }

        struct npc_rinling_darkmoonAI: public ScriptedAI
        {
            npc_rinling_darkmoonAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 nextQuoteTimer;
            uint8 nextQuote;

            uint32 nextTargetChangeTimer;
            int8 targetIndex;
            uint64 targetList[3];

            void Reset()
            {
                nextQuoteTimer = urand(1, 60)*1000;
                nextQuote = 0;
                nextTargetChangeTimer = 0;
                targetIndex = -1;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextQuoteTimer <= diff)
                {
                    nextQuoteTimer = 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(rinlingQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= RINLING_QUOTES_TOTAL)
                        nextQuote = 0;
                }
                else
                    nextQuoteTimer -= diff;

                if (nextTargetChangeTimer <= diff)
                {
                    if (targetIndex < 0 || targetIndex >= 3)
                    {
                        std::list<Creature*> crList;
                        GetCreatureListWithEntryInGrid(crList, me, 24171, 100.0f);
                        int i = 0;
                        for (std::list<Creature*>::const_iterator itr = crList.begin(); itr != crList.end(); ++itr)
                        {
                            targetList[i++] = (*itr)->GetGUID();
                            if (i >= 3)
                                break;
                        }
                    }

                    int8 chosen = -1;
                    do
                    {
                        // randomly choose one of targets
                        chosen = urand(0,2);
                    } while (chosen == targetIndex && urand(0,1) == 0); // the same target could be chosen with only 50% chance (+-)

                    targetIndex = chosen;

                    Creature* cr = Creature::GetCreature(*me, targetList[chosen]);
                    if (cr)
                        cr->CastSpell(cr, SPELL_HESHOOTS_TARGET_INDICATOR, true);

                    nextTargetChangeTimer = 4500 + urand(0,1000);
                }
                else
                    nextTargetChangeTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_rinling_darkmoonAI(c);
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, RINLING_GOSSIP_1_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, RINLING_GOSSIP_1_START, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(RINLING_TEXT_ID_1, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Info
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, RINLING_GOSSIP_2_UNDERSTAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->SEND_GOSSIP_MENU(RINLING_TEXT_ID_2, pCreature->GetGUID());
                return true;
            }
            // Ready to play
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                if (pPlayer->GetItemCount(ITEM_DARKMOON_GAME_TOKEN, false) > 0)
                {
                    pPlayer->DestroyItemCount(ITEM_DARKMOON_GAME_TOKEN, 1, true);

                    pPlayer->CastSpell(pPlayer, SPELL_HESHOOTS_CRACKSHOT_ENABLE, true);

                    int16 progress = pPlayer->GetReqKillOrCastCurrentCount(QUEST_HE_SHOOTS_HE_SCORES, 54231);
                    if (progress > 0)
                        pPlayer->SetPower(POWER_SCRIPTED, progress);
                }
                else
                {
                    pPlayer->GetSession()->SendNotification("You don't have enough Darkmoon Game Tokens!");
                }
                return true;
            }
            // "I understand"
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                return OnGossipHello(pPlayer, pCreature);
            }

            return false;
        }
};

class spell_heshoots_shoot_hit: public SpellScriptLoader
{
    public:
        spell_heshoots_shoot_hit(): SpellScriptLoader("spell_heshoots_shoot_hit") { }

        class spell_heshoots_shoot_hit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_heshoots_shoot_hit_SpellScript);

            void HitHandler()
            {
                if (!GetCaster())
                    return;

                Unit* target = GetHitUnit();

                if (!target)
                    return;

                if (Aura* shootAura = target->GetAura(SPELL_HESHOOTS_TARGET_INDICATOR))
                {
                    target->CastSpell(GetCaster(), SPELL_HESHOOTS_KILL_CREDIT, true);

                    Player* pl = GetCaster()->ToPlayer();

                    if (shootAura->GetMaxDuration() - shootAura->GetDuration() < 1000)
                    {
                        target->CastSpell(GetCaster(), SPELL_HESHOOTS_KILL_CREDIT_BONUS, true);
                        AchievementEntry const* achiev = sAchievementStore.LookupEntry(ACHIEVEMENT_QUICK_SHOT);
                        if (pl)
                            pl->CompletedAchievement(achiev);
                    }

                    if (pl)
                    {
                        if (pl->GetReqKillOrCastCurrentCount(QUEST_HE_SHOOTS_HE_SCORES, 54231) >= 25)
                            pl->RemoveAurasDueToSpell(SPELL_HESHOOTS_CRACKSHOT_ENABLE);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_heshoots_shoot_hit_SpellScript::HitHandler);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_heshoots_shoot_hit_SpellScript();
        }
};

class spell_heshoots_indicator: public SpellScriptLoader
{
    public:
        spell_heshoots_indicator(): SpellScriptLoader("spell_heshoots_indicator")
        {
        }

        class spell_heshoots_indicator_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_heshoots_indicator_AuraScript);

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_HESHOOTS_TARGET_INDICATOR);
            }

            bool Load()
            {
                return true;
            }

            void EffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner())
                    return;

                GetUnitOwner()->CastSpell(GetUnitOwner(), SPELL_HESHOOTS_TARGET_INDICATOR_VISUAL, true);
            }

            void EffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner())
                    return;

                GetUnitOwner()->RemoveAurasDueToSpell(SPELL_HESHOOTS_TARGET_INDICATOR_VISUAL);
            }

            void Register()
            {
                OnEffectApply +=  AuraEffectApplyFn(spell_heshoots_indicator_AuraScript::EffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectApplyFn(spell_heshoots_indicator_AuraScript::EffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_heshoots_indicator_AuraScript();
        }
};

/*

SQL

UPDATE quest_template SET ReqSpellCast1=0 WHERE entry=29438;
UPDATE creature_template SET ScriptName = 'npc_rinling_darkmoon', AIName = '' WHERE entry = 14841;
UPDATE creature_template SET modelid1 = 11686, modelid2 = 0, modelid3 = 0, modelid4 = 0 WHERE entry = 24171;
REPLACE INTO conditions VALUES (13, 0, 101872, 0, 18, 1, 24171, 0, 0, '', 'Shoot - implicit target Target bunny');
DELETE FROM spell_script_names WHERE spell_id IN (101872, 101010);
INSERT INTO spell_script_names VALUES (101872, 'spell_heshoots_shoot_hit'), (101010, 'spell_heshoots_indicator');

*/

/*
 * Quest: It's Hammer Time
 */

#define MOLA_QUOTE_1 "Hammer swingin' fun! Who wants to swing a hammer?"
#define MOLA_QUOTE_2 "Hammer time!"
#define MOLA_QUOTE_3 "Swing a hammer, win a prize!"
#define MOLA_QUOTE_4 "Whack some gnolls! Step right up and whack some gnolls!"

#define MOLA_QUOTES_TOTAL 4
const char* molaQuotes[MOLA_QUOTES_TOTAL] = {
    MOLA_QUOTE_1, MOLA_QUOTE_2, MOLA_QUOTE_3, MOLA_QUOTE_4
};

#define MOLA_TEXT_ID_TITLE 120008
#define MOLA_GOSSIP_INFO_TEXT  "How do I play Whack-a-Gnoll?"
#define MOLA_GOSSIP_WHACK_TEXT "Ready to whack! |cFF0000FF(Darkmoon Game Token)|r"

#define MOLA_TEXT_ID_INFO 120009
#define MOLA_GOSSIP_UNDERSTAND_TEXT "I understand"

#define GNOLL_POSITION_COUNT 9

const Position gnollPositions[GNOLL_POSITION_COUNT] = {
    {-3983.67f, 6300.26f, 13.2001f, 3.8f},
    {-3988.96f, 6296.32f, 13.2001f, 3.8f},
    {-3994.49f, 6292.84f, 13.2001f, 3.8f},
    {-3979.42f, 6295.16f, 13.2001f, 3.8f},
    {-3985.05f, 6291.41f, 13.2001f, 3.8f},
    {-3990.7f,  6287.61f, 13.2001f, 3.8f},
    {-3975.94f, 6289.36f, 13.2001f, 3.8f},
    {-3981.64f, 6285.74f, 13.2001f, 3.8f},
    {-3986.78f, 6282.54f, 13.2001f, 3.8f},
};

enum GnollPuppetEntries
{
    PUPPET_GNOLL_NORMAL = 54444,
    PUPPET_GNOLL_BONUS  = 54549,
    PUPPET_BABY         = 54466
};

enum WhackSpells
{
    SPELL_WHACKAGNOLL_ENABLE        = 110230,
    SPELL_WHACKAGNOLL_SPELLBAR      = 101612,
    SPELL_WHACKAGNOLL_KILL_CREDIT   = 101835,
    SPELL_WHACKAGNOLL_STUN          = 101679,
    SPELL_WHACKAGNOLL_WHACK_DUMMY   = 101604,
    SPELL_WHACKAGNOLL_WHACK_HIT     = 102022,
    SPELL_WHACKAGNOLL_SPAWN_VISUAL  = 102136,
    SPELL_WHACKAGNOLL_STAY_OUT      = 109977,
};

struct BarrelData
{
    uint64 creatureGuid;
    uint32 despawnTimer;
    uint32 nextSpawnTimer;
};

class npc_darkmoon_mola: public CreatureScript
{
    public:
        npc_darkmoon_mola(): CreatureScript("npc_darkmoon_mola")
        {
        }

        struct npc_darkmoon_molaAI: public ScriptedAI
        {
            npc_darkmoon_molaAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            BarrelData barrels[GNOLL_POSITION_COUNT];
            uint32 nextQuoteTimer;
            uint8 nextQuote;

            void Reset()
            {
                memset(&barrels, 0, sizeof(barrels));
                for (uint32 i = 0; i < GNOLL_POSITION_COUNT; i++)
                    barrels[i].nextSpawnTimer = urand(0,6000);

                nextQuoteTimer = urand(1, 60)*1000;
                nextQuote = 0;
            }

            uint32 RollCreatureEntry()
            {
                // 60% normal gnoll, 30% baby, 10% bonus gnoll
                uint32 rand = urand(1,100);
                if (rand <= 60)
                    return PUPPET_GNOLL_NORMAL;
                if (rand <= 90)
                    return PUPPET_BABY;
                return PUPPET_GNOLL_BONUS;
            }

            void UpdateAI(const uint32 diff)
            {
                Creature* tmp;

                for (uint32 i = 0; i < GNOLL_POSITION_COUNT; i++)
                {
                    if (barrels[i].despawnTimer != 0)
                    {
                        if (barrels[i].despawnTimer <= diff)
                        {
                            tmp = Creature::GetCreature(*me, barrels[i].creatureGuid);
                            if (tmp)
                            {
                                tmp->Kill(tmp);
                                tmp->ForcedDespawn(1000);
                            }

                            barrels[i].creatureGuid = 0;
                            barrels[i].despawnTimer = 0;
                        }
                        else
                            barrels[i].despawnTimer -= diff;
                    }

                    if (barrels[i].nextSpawnTimer <= diff)
                    {
                        tmp = me->SummonCreature(RollCreatureEntry(), gnollPositions[i], TEMPSUMMON_MANUAL_DESPAWN, 0, 0);
                        tmp->SetPhaseMask(2, true);
                        tmp->CastSpell(tmp, SPELL_WHACKAGNOLL_SPAWN_VISUAL, true);
                        tmp->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        barrels[i].creatureGuid = tmp->GetGUID();
                        barrels[i].despawnTimer = urand(3500,5000);
                        barrels[i].nextSpawnTimer = urand(5000,8000);
                    }
                    else
                        barrels[i].nextSpawnTimer -= diff;
                }

                if (nextQuoteTimer <= diff)
                {
                    nextQuoteTimer = 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(molaQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= MOLA_QUOTES_TOTAL)
                        nextQuote = 0;
                }
                else
                    nextQuoteTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_darkmoon_molaAI(c);
        }


        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MOLA_GOSSIP_INFO_TEXT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MOLA_GOSSIP_WHACK_TEXT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(MOLA_TEXT_ID_TITLE, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Info
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, MOLA_GOSSIP_UNDERSTAND_TEXT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->SEND_GOSSIP_MENU(MOLA_TEXT_ID_INFO, pCreature->GetGUID());
                return true;
            }
            // Whack
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                if (pPlayer->GetItemCount(ITEM_DARKMOON_GAME_TOKEN, false) > 0)
                {
                    pPlayer->DestroyItemCount(ITEM_DARKMOON_GAME_TOKEN, 1, true);

                    pPlayer->CastSpell(pPlayer, SPELL_WHACKAGNOLL_ENABLE, true);
                    pPlayer->CastSpell(pPlayer, SPELL_WHACKAGNOLL_SPELLBAR, true);

                    int16 progress = pPlayer->GetReqKillOrCastCurrentCount(QUEST_ITS_HAMMER_TIME, 54505);
                    if (progress > 0)
                        pPlayer->SetPower(POWER_SCRIPTED, progress);
                }
                else
                {
                    pPlayer->GetSession()->SendNotification("You don't have enough Darkmoon Game Tokens!");
                }
                return true;
            }
            // "I understand"
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                return OnGossipHello(pPlayer, pCreature);
            }

            return false;
        }
};

class spell_whackagnoll_whack_dummy: public SpellScriptLoader
{
    public:
        spell_whackagnoll_whack_dummy(): SpellScriptLoader("spell_whackagnoll_whack_dummy") { }

        class spell_whackagnoll_whack_dummy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_whackagnoll_whack_dummy_SpellScript);

            void HitHandler()
            {
                if (!GetCaster())
                    return;

                GetCaster()->CastSpell(GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ(), SPELL_WHACKAGNOLL_WHACK_HIT, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_whackagnoll_whack_dummy_SpellScript::HitHandler);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_whackagnoll_whack_dummy_SpellScript();
        }
};

class spell_whackagnoll_whack_hit: public SpellScriptLoader
{
    public:
        spell_whackagnoll_whack_hit(): SpellScriptLoader("spell_whackagnoll_whack_hit") { }

        class spell_whackagnoll_whack_hit_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_whackagnoll_whack_hit_SpellScript);

            void AfterHitHandler()
            {
                if (!GetCaster())
                    return;

                Unit* target = GetHitUnit();

                if (!target || target->isDead())
                    return;

                if (target->GetTypeId() != TYPEID_UNIT)
                    return;

                Player* pl = GetCaster()->ToPlayer();
                if (!pl)
                    return;

                switch (target->GetEntry())
                {
                    case PUPPET_GNOLL_NORMAL:
                    {
                        pl->CastSpell(pl, SPELL_WHACKAGNOLL_KILL_CREDIT, true);
                        target->Kill(target);
                        break;
                    }
                    case PUPPET_GNOLL_BONUS:
                    {
                        for (uint32 i = 0; i < 3; i++)
                            pl->CastSpell(pl, SPELL_WHACKAGNOLL_KILL_CREDIT, true);
                        target->Kill(target);
                        break;
                    }
                    case PUPPET_BABY:
                    {
                        pl->CastSpell(pl, SPELL_WHACKAGNOLL_STUN, true);
                        target->Kill(target);
                        break;
                    }
                }

                if (pl->GetReqKillOrCastCurrentCount(QUEST_ITS_HAMMER_TIME, 54505) >= 30)
                {
                    pl->RemoveAurasDueToSpell(SPELL_WHACKAGNOLL_ENABLE);
                    pl->RemoveAurasDueToSpell(SPELL_WHACKAGNOLL_SPELLBAR);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_whackagnoll_whack_hit_SpellScript::AfterHitHandler);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_whackagnoll_whack_hit_SpellScript();
        }
};

class spell_whackagnoll_enable: public SpellScriptLoader
{
    public:
        spell_whackagnoll_enable(): SpellScriptLoader("spell_whackagnoll_enable")
        {
        }

        class spell_whackagnoll_enable_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_whackagnoll_enable_AuraScript);

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_WHACKAGNOLL_ENABLE);
            }

            bool Load()
            {
                return true;
            }

            void EffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner() || GetUnitOwner()->GetTypeId() != TYPEID_PLAYER)
                    return;

                Unit* owner = GetUnitOwner();
                owner->SetPhaseMask(owner->GetPhaseMask() | 2, true);
            }

            void EffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner() || GetUnitOwner()->GetTypeId() != TYPEID_PLAYER)
                    return;

                Unit* owner = GetUnitOwner();
                if (owner->GetPhaseMask() & 2)
                    owner->SetPhaseMask(owner->GetPhaseMask() - 2, true);
            }

            void Register()
            {
                OnEffectApply +=  AuraEffectApplyFn(spell_whackagnoll_enable_AuraScript::EffectApply, EFFECT_0, SPELL_AURA_369, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectApplyFn(spell_whackagnoll_enable_AuraScript::EffectRemove, EFFECT_0, SPELL_AURA_369, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_whackagnoll_enable_AuraScript();
        }
};

class npc_darkmoon_whack_controller: public CreatureScript
{
    public:
        npc_darkmoon_whack_controller(): CreatureScript("npc_darkmoon_whack_controller")
        {
        }

        struct npc_darkmoon_whack_controllerAI: public ScriptedAI
        {
            npc_darkmoon_whack_controllerAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint32 nextCheckTimer;

            void Reset()
            {
                nextCheckTimer = 1000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextCheckTimer <= diff)
                {
                    Map::PlayerList const& plList = me->GetMap()->GetPlayers();
                    Player* tmp;
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    {
                        tmp = itr->getSource();
                        if (!tmp)
                            continue;

                        // is further - dont care
                        if (!tmp->IsWithinDist2d(me->GetPositionX(), me->GetPositionY(), 15.0f))
                            continue;

                        // has enable aura - dont care
                        if (tmp->HasAura(SPELL_WHACKAGNOLL_ENABLE))
                            continue;

                        tmp->CastSpell(tmp, SPELL_WHACKAGNOLL_STAY_OUT, true);
                    }

                    nextCheckTimer = 1000;
                }
                else
                    nextCheckTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_darkmoon_whack_controllerAI(c);
        }
};

/*

SQL

UPDATE creature_template SET ScriptName = 'npc_darkmoon_mola', AIName = '' WHERE entry=54601;
UPDATE creature_template SET ScriptName = 'npc_darkmoon_whack_controller', AIName = '' WHERE entry=58570;
DELETE FROM spell_script_names WHERE spell_id IN (101604, 102022, 110230);
INSERT INTO spell_script_names VALUES (101604, 'spell_whackagnoll_whack_dummy'), (102022, 'spell_whackagnoll_whack_hit'), (110230, 'spell_whackagnoll_enable');
REPLACE INTO conditions VALUES (13, 0, 102022, 0, 18, 1, 54444, 0, 0, '', 'Whack - implicit targets gnoll');
REPLACE INTO conditions VALUES (13, 0, 102022, 0, 18, 1, 54549, 0, 0, '', 'Whack - implicit targets hogger');
REPLACE INTO conditions VALUES (13, 0, 102022, 0, 18, 1, 54466, 0, 0, '', 'Whack - implicit targets baby');
INSERT INTO spell_target_position VALUES (109977, 974, -4008.035156, 6270.989258, 11.879892, 0.702977);

*/

/*
 * Quest: Tonk Commander
 */

enum TonkSpells
{
    SPELL_TONK_ENABLE       = 102178,
    SPELL_TONK_SHOOT        = 102292,
    SPELL_TONK_NITRO_BOOST  = 102297,
    SPELL_TONK_KILL_CREDIT  = 110162,
    SPELL_TONK_STAY_OUT     = 109976,

    SPELL_ENEMY_TONK_MARKED         = 102341,
    SPELL_ENEMY_TONK_CANNON_BLAST   = 102227,
};

enum TonkNPCs
{
    NPC_PLAYER_TONK         = 54588,
    NPC_ENEMY_TONK          = 54642,
    NPC_ENEMY_MINIZEP       = 54643,
    NPC_TONK_TARGET         = 33081,
};

#define TONK_TARGET_POSITION_COUNT 33

const Position tonkTargetPositions[TONK_TARGET_POSITION_COUNT] = {
    {-4145.988770f, 6290.719727f, 13.116899f, 0.499545f},
    {-4137.421387f, 6289.625977f, 13.116899f, 1.210661f},
    {-4133.106934f, 6296.116699f, 13.116899f, 2.729411f},
    {-4139.067383f, 6300.952148f, 13.116899f, 0.894536f},
    {-4139.654297f, 6305.813477f, 13.116899f, 0.671786f},
    {-4143.446777f, 6308.530762f, 13.116899f, 2.021786f},
    {-4148.482910f, 6308.563965f, 13.116899f, 0.614411f},
    {-4147.683105f, 6300.739258f, 13.116899f, 0.972161f},
    {-4145.276855f, 6304.266602f, 13.116899f, 0.972161f},
    {-4142.250000f, 6312.737793f, 13.116899f, 0.297161f},
    {-4137.417480f, 6316.265137f, 13.116899f, 6.023472f},
    {-4133.054688f, 6318.007813f, 13.116899f, 5.348472f},
    {-4127.991211f, 6313.178223f, 13.116899f, 2.551661f},
    {-4125.186035f, 6313.233398f, 13.116899f, 3.395411f},
    {-4122.878418f, 6307.192383f, 13.116899f, 3.395411f},
    {-4126.788574f, 6306.019043f, 13.116899f, 2.274911f},
    {-4124.875977f, 6300.727051f, 13.116899f, 3.176036f},
    {-4123.939941f, 6299.044434f, 13.116899f, 2.328911f},
    {-4126.655762f, 6294.246094f, 13.116899f, 2.328911f},
    {-4129.494141f, 6294.895020f, 13.116899f, 1.205036f},
    {-4131.038086f, 6292.043457f, 13.116899f, 2.575286f},
    {-4135.638184f, 6288.404785f, 13.116899f, 1.750661f},
    {-4137.863770f, 6292.131836f, 13.116899f, 1.075661f},
    {-4137.863770f, 6297.746094f, 13.116899f, 1.755162f},
    {-4135.811523f, 6302.905762f, 13.116517f, 1.755162f},
    {-4133.516113f, 6307.136719f, 13.116517f, 1.755162f},
    {-4143.302246f, 6310.689941f, 13.116517f, 0.573911f},
    {-4148.054688f, 6297.366211f, 13.116712f, 5.652222f},
    {-4139.401855f, 6294.312500f, 13.116712f, 0.047411f},
    {-4119.912109f, 6299.708496f, 13.116712f, 4.221162f},
    {-4120.738281f, 6306.735352f, 13.116712f, 3.097287f},
    {-4125.261719f, 6317.238281f, 13.116712f, 3.154662f},
    {-4135.285645f, 6321.727539f, 13.116712f, 4.992911f}
};

const Position playerSpawnPosition  = {-4130.196777f, 6320.329102f, 13.116393f, 4.368710f};
const Position playerKickPosition   = {-4125.278809f, 6332.548828f, 12.219045f, 4.313733f};
const Position circleCenterPosition = {-4135.632813f, 6302.201172f, 13.116685f, 1.336366f};
const float circleBattleRadius      = 17.0f;

#define FINLAY_QUOTE_1 "Hey, hey! Command a tonk in glorious battle!"
#define FINLAY_QUOTE_2 "Step right up and try a tonk!"
#define FINLAY_QUOTE_3 "Tonks! We got tonks here!"
#define FINLAY_QUOTE_4 "We're under attack! Step up and do your part!"

#define FINLAY_QUOTES_TOTAL 4
const char* finlayQuotes[FINLAY_QUOTES_TOTAL] = {
    FINLAY_QUOTE_1, FINLAY_QUOTE_2, FINLAY_QUOTE_3, FINLAY_QUOTE_4
};

#define FINLAY_TEXT_ID_1 120010
#define FINLAY_GOSSIP_1_INFO "How do I play the Tonk Challenge?"
#define FINLAY_GOSSIP_1_LAUNCH "Ready to play! |cFF0000FF(Darkmoon Game Token)|r"
#define FINLAY_TEXT_ID_2 120011
#define FINLAY_GOSSIP_2_UNDERSTAND "I understand"

struct TargetSpawn
{
    uint64 guid;
    uint8 pos;
    uint32 nextSpawnTime;
    uint32 nextCheckTime;
};

#define TONK_TARGETS_MAX 8

class npc_finlay_darkmoon: public CreatureScript
{
    public:
        npc_finlay_darkmoon(): CreatureScript("npc_finlay_darkmoon")
        {
        }

        struct npc_finlay_darkmoonAI: public ScriptedAI
        {
            npc_finlay_darkmoonAI(Creature* c): ScriptedAI(c)
            {
                spawnedTonks = false;
                Reset();
            }

            uint32 nextQuoteTimer;
            uint32 nextCheckTimer;
            uint8 nextQuote;
            TargetSpawn targetSpawns[TONK_TARGETS_MAX];
            bool spawnedTonks;

            void Reset()
            {
                nextQuoteTimer = urand(1, 60)*1000;
                nextCheckTimer = 1000;
                nextQuote = 0;

                memset(&targetSpawns, 0, sizeof(targetSpawns));

                if (!spawnedTonks)
                {
                    uint8 point;
                    for (uint32 i = 0; i < 3; i++)
                    {
                        point = urand(0, TONK_TARGET_POSITION_COUNT);
                        me->SummonCreature(NPC_ENEMY_TONK, tonkTargetPositions[point], TEMPSUMMON_MANUAL_DESPAWN);
                    }
                    spawnedTonks = true;
                }
            }

            bool IsSpawnAtPos(uint8 pos)
            {
                for (uint32 i = 0; i < TONK_TARGETS_MAX; i++)
                {
                    if (pos == targetSpawns[i].pos)
                        return true;
                }
                return false;
            }

            void UpdateAI(const uint32 diff)
            {
                if (nextQuoteTimer <= diff)
                {
                    nextQuoteTimer = 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(finlayQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= FINLAY_QUOTES_TOTAL)
                        nextQuote = 0;
                }
                else
                    nextQuoteTimer -= diff;

                if (nextCheckTimer <= diff)
                {
                    Map::PlayerList const& plList = me->GetMap()->GetPlayers();
                    Player* tmp;
                    for (Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                    {
                        tmp = itr->getSource();
                        if (!tmp)
                            continue;

                        // is further - dont care
                        if (!tmp->IsWithinDist2d(&circleCenterPosition, circleBattleRadius))
                            continue;

                        // has enable aura - dont care
                        if (tmp->HasAura(SPELL_TONK_ENABLE))
                            continue;

                        tmp->CastSpell(tmp, SPELL_TONK_STAY_OUT, true);
                    }

                    nextCheckTimer = 500;
                }
                else
                    nextCheckTimer -= diff;

                for (uint32 i = 0; i < TONK_TARGETS_MAX; i++)
                {
                    if (targetSpawns[i].nextCheckTime <= diff)
                    {
                        Creature* cr = Creature::GetCreature(*me, targetSpawns[i].guid);
                        if (!cr || cr->isDead())
                        {
                            targetSpawns[i].nextSpawnTime = 2000;
                            targetSpawns[i].nextCheckTime = 5000;
                        }
                        else
                        {
                            targetSpawns[i].nextCheckTime = 2000;
                        }
                    }
                    else
                        targetSpawns[i].nextCheckTime -= diff;

                    if (targetSpawns[i].nextSpawnTime <= diff)
                    {
                        targetSpawns[i].nextSpawnTime = urand(20000, 40000);
                        if (Creature* cr = Creature::GetCreature(*me, targetSpawns[i].guid))
                        {
                            cr->ForcedDespawn(1000);
                        }

                        uint8 chosen = 0;
                        do
                        {
                            chosen = urand(0, TONK_TARGET_POSITION_COUNT);
                        } while (IsSpawnAtPos(chosen));

                        targetSpawns[i].pos = chosen;

                        Creature* cr = me->SummonCreature(NPC_TONK_TARGET, tonkTargetPositions[chosen], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 1000);
                        if (cr)
                            targetSpawns[i].guid = cr->GetGUID();
                    }
                    else
                        targetSpawns[i].nextSpawnTime -= diff;
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_finlay_darkmoonAI(c);
        }

        bool OnGossipHello(Player* pPlayer, Creature* pCreature)
        {
            if (pCreature->isQuestGiver())
                pPlayer->PrepareQuestMenu(pCreature->GetGUID());

            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, FINLAY_GOSSIP_1_INFO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, FINLAY_GOSSIP_1_LAUNCH, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

            pPlayer->SEND_GOSSIP_MENU(FINLAY_TEXT_ID_1, pCreature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
        {
            pPlayer->CLOSE_GOSSIP_MENU();

            // Info
            if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, FINLAY_GOSSIP_2_UNDERSTAND, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                pPlayer->SEND_GOSSIP_MENU(FINLAY_TEXT_ID_2, pCreature->GetGUID());
                return true;
            }
            // Play
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+2)
            {
                if (pPlayer->GetItemCount(ITEM_DARKMOON_GAME_TOKEN, false) > 0)
                {
                    pPlayer->DestroyItemCount(ITEM_DARKMOON_GAME_TOKEN, 1, true);

                    Creature* summon = pPlayer->SummonCreature(NPC_PLAYER_TONK, playerSpawnPosition, TEMPSUMMON_MANUAL_DESPAWN, 0);
                    if (summon)
                    {
                        pPlayer->EnterVehicle(summon);
                        pPlayer->CastSpell(pPlayer, SPELL_TONK_ENABLE, true);

                        int16 progress = pPlayer->GetReqKillOrCastCurrentCount(QUEST_TONK_COMMANDER, 33081);
                        if (progress > 0)
                            pPlayer->SetPower(POWER_SCRIPTED, progress);
                    }
                }
                else
                {
                    pPlayer->GetSession()->SendNotification("You don't have enough Darkmoon Game Tokens!");
                }
                return true;
            }
            // "I understand"
            else if (uiAction == GOSSIP_ACTION_INFO_DEF+3)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                return OnGossipHello(pPlayer, pCreature);
            }

            return false;
        }
};

class spell_tonk_enable: public SpellScriptLoader
{
    public:
        spell_tonk_enable(): SpellScriptLoader("spell_tonk_enable")
        {
        }

        class spell_tonk_enable_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_tonk_enable_AuraScript);

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_TONK_ENABLE);
            }

            bool Load()
            {
                return true;
            }

            void EffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner() || GetUnitOwner()->GetTypeId() != TYPEID_PLAYER)
                    return;

                //Unit* owner = GetUnitOwner();
                //owner->SetPhaseMask(owner->GetPhaseMask() | 2, true);
            }

            void EffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*modes*/)
            {
                if (!GetUnitOwner() || GetUnitOwner()->GetTypeId() != TYPEID_PLAYER)
                    return;

                Unit* owner = GetUnitOwner();
                //if (owner->GetPhaseMask() & 2)
                //    owner->SetPhaseMask(owner->GetPhaseMask() - 2, true);
                if (Creature* vb = owner->ToPlayer()->GetVehicleCreatureBase())
                {
                    vb->Kill(vb);
                    vb->ForcedDespawn();
                }

                owner->NearTeleportTo(playerKickPosition.GetPositionX(), playerKickPosition.GetPositionY(), playerKickPosition.GetPositionZ(), playerKickPosition.GetOrientation(), false);
            }

            void Register()
            {
                OnEffectApply +=  AuraEffectApplyFn(spell_tonk_enable_AuraScript::EffectApply, EFFECT_0, SPELL_AURA_369, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectApplyFn(spell_tonk_enable_AuraScript::EffectRemove, EFFECT_0, SPELL_AURA_369, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_tonk_enable_AuraScript();
        }
};

class npc_darkmoon_tonk_target: public CreatureScript
{
    public:
        npc_darkmoon_tonk_target(): CreatureScript("npc_darkmoon_tonk_target")
        {
        }

        struct npc_darkmoon_tonk_targetAI: public ScriptedAI
        {
            npc_darkmoon_tonk_targetAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            void SpellHit(Unit* source, const SpellEntry* spell)
            {
                if (spell->Id == SPELL_TONK_SHOOT)
                {
                    Vehicle* veh = source->GetVehicleKit();
                    if (veh)
                    {
                        for (uint32 i = 0; i < MAX_VEHICLE_SEATS; i++)
                        {
                            if (Unit* passenger = veh->GetPassenger(i))
                                passenger->CastSpell(passenger, SPELL_TONK_KILL_CREDIT, true);
                        }
                    }
                    me->CastSpell(me, 100626, true);
                    me->ForcedDespawn(600);
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_darkmoon_tonk_targetAI(c);
        }
};

class vehicle_darkmoon_steam_tonk: public VehicleScript
{
    public:
        vehicle_darkmoon_steam_tonk(): VehicleScript("vehicle_darkmoon_steam_tonk")
        {
        }

        void OnRemovePassenger(Vehicle* veh, Unit* passenger)
        {
            if (passenger)
            {
                passenger->GetMotionMaster()->MovementExpired();
                passenger->RemoveAurasDueToSpell(SPELL_TONK_ENABLE);
            }

            if (Unit* base = veh->GetBase())
                if (base->ToCreature())
                    base->ToCreature()->ForcedDespawn(0);
        }
};

class npc_darkmoon_enemy_tonk: public CreatureScript
{
    public:
        npc_darkmoon_enemy_tonk(): CreatureScript("npc_darkmoon_enemy_tonk")
        {
        }

        struct npc_darkmoon_enemy_tonkAI: public ScriptedAI
        {
            npc_darkmoon_enemy_tonkAI(Creature* c): ScriptedAI(c)
            {
                Reset();
            }

            uint64 target;
            bool changePath;
            uint32 pathCheckTimer;
            uint32 targetCheckTimer;

            void Reset()
            {
                target = 0;
                changePath = true;
                pathCheckTimer = 0;
                targetCheckTimer = 1000;
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (id == 0)
                    changePath = true;
            }

            void UpdateAI(const uint32 diff)
            {
                if (pathCheckTimer != 0)
                {
                    if (pathCheckTimer <= diff)
                    {
                        target = 0;
                        changePath = true;
                        pathCheckTimer = 0;
                    }
                    else
                        pathCheckTimer -= diff;
                }

                if (changePath)
                {
                    uint8 point = urand(0, TONK_TARGET_POSITION_COUNT);
                    me->GetMotionMaster()->MovePoint(0, tonkTargetPositions[point]);
                    changePath = false;
                }

                if (targetCheckTimer <= diff)
                {
                    if (target == 0)
                    {
                        std::list<Creature*> crList;
                        GetCreatureListWithEntryInGrid(crList, me, NPC_PLAYER_TONK, 4.5f);
                        for (std::list<Creature*>::iterator itr = crList.begin(); itr != crList.end(); ++itr)
                        {
                            if ((*itr)->isInFront(me, 7.0f, M_PI/1.5f) && !(*itr)->HasAura(SPELL_ENEMY_TONK_MARKED))
                            {
                                target = (*itr)->GetGUID();
                                me->StopMoving();
                                me->GetMotionMaster()->MovementExpired(true);
                                me->GetMotionMaster()->MoveIdle();
                                changePath = false;
                                me->SetFacingToObject(*itr);
                                me->AddAura(SPELL_ENEMY_TONK_MARKED, (*itr));
                                me->CastSpell((*itr), SPELL_ENEMY_TONK_CANNON_BLAST, false);
                                pathCheckTimer = 3000; // 2s spellcast + 1s delay
                                break;
                            }
                        }

                        if (target == 0)
                            targetCheckTimer = 1000;
                        else
                            targetCheckTimer = 2500;
                    }
                    else
                    {
                        Unit* targetUnit = Unit::GetUnit(*me, target);
                        if (targetUnit && targetUnit->IsWithinDist(me, 7.0f, true) && targetUnit->isInFront(me, 7.0f, M_PI/1.5f))
                        {
                            targetUnit->CastSpell(targetUnit, 100626, true);
                            me->DealDamage(targetUnit, (targetUnit->GetMaxHealth() / 2) + 1);
                        }
                        targetCheckTimer = 2000;
                    }
                }
                else
                    targetCheckTimer -= diff;
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_darkmoon_enemy_tonkAI(c);
        }
};

/*

SQL

UPDATE creature_template SET ScriptName='npc_finlay_darkmoon', AIName='' WHERE entry=54605;
UPDATE creature_template SET ScriptName='npc_darkmoon_tonk_target', AIName='' WHERE entry=33081;
UPDATE creature_template SET ScriptName='npc_darkmoon_enemy_tonk', AIName='' WHERE entry=54642;
UPDATE creature_template SET VehicleId=1734,spell1=102292,spell2=102297,ScriptName='vehicle_darkmoon_steam_tonk' WHERE entry=54588;
REPLACE INTO conditions VALUES (13, 0, 102292, 0, 18, 1, 33081, 0, 0, '', 'Cannon shoot - implicit targets disc');
-- TODO: find out, whether the tonk and minizep is targettable by player tonk attack
-- REPLACE INTO conditions VALUES (13, 0, 102292, 0, 18, 1, 54642, 0, 0, '', 'Cannon shoot - implicit targets tonk');
-- REPLACE INTO conditions VALUES (13, 0, 102292, 0, 18, 1, 54643, 0, 0, '', 'Cannon shoot - implicit targets minizep');
DELETE FROM spell_script_names WHERE spell_id IN (102178);
INSERT INTO spell_script_names VALUES (102178, 'spell_tonk_enable');
REPLACE INTO spell_target_position VALUES (109976, 974, -4125.278809, 6332.548828, 12.219045, 4.313733);

*/

void AddSC_darkmoon_island()
{
    new npc_maxima_darkmoon();
    new npc_cannonball_bunny();
    new spell_cannon_prep();
    new npc_teleporter_fozlebub();

    new npc_jessica_darkmoon();
    new spell_ring_toss_enable();
    new spell_ring_toss_trigger();
    new spell_ring_toss_hit();

    new npc_rinling_darkmoon();
    new spell_heshoots_shoot_hit();
    new spell_heshoots_indicator();

    new npc_darkmoon_mola();
    new spell_whackagnoll_whack_dummy();
    new spell_whackagnoll_whack_hit();
    new spell_whackagnoll_enable();
    new npc_darkmoon_whack_controller();

    new npc_finlay_darkmoon();
    new spell_tonk_enable();
    new npc_darkmoon_tonk_target();
    new vehicle_darkmoon_steam_tonk();
    new npc_darkmoon_enemy_tonk();
}
