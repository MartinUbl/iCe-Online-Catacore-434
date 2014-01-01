/**
 * Darkmoon island scripts
 *
 * Copyright (c) iCe Online, 2006-2013
 *
 **/

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

            uint32 nextQuoteTime;
            uint8 nextQuote;

            void Reset()
            {
                nextQuoteTime = getMSTime() + urand(1, 60)*1000;
                nextQuote = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (getMSTime() > nextQuoteTime)
                {
                    nextQuoteTime = getMSTime() + 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(maximaQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= MAXIMA_QUOTES_TOTAL)
                        nextQuote = 0;
                }
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

            uint32 nextCheckTime;

            void Reset()
            {
                nextCheckTime = getMSTime() + 1000;
            }

            void UpdateAI(const uint32 diff)
            {
                if (getMSTime() > nextCheckTime)
                {
                    nextCheckTime = getMSTime() + 1000;

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

/*

SQL

UPDATE creature_template SET ScriptName = 'npc_maxima_darkmoon', AIName = '' WHERE entry = 15303;
UPDATE creature_template SET ScriptName = 'npc_cannonball_bunny', AIName = '' WHERE entry = 33068;
DELETE FROM spell_script_names WHERE spell_id = 102112;
INSERT INTO spell_script_names VALUES (102112, 'spell_cannon_prep');

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

            uint32 nextQuoteTime;
            uint8 nextQuote;

            void Reset()
            {
                nextQuoteTime = getMSTime() + urand(1, 60)*1000;
                nextQuote = 0;
            }

            void UpdateAI(const uint32 diff)
            {
                if (getMSTime() > nextQuoteTime)
                {
                    nextQuoteTime = getMSTime() + 30000 + urand(0, 15)*10000; // 30 - 180s (step by 10s, not so big importance)
                    me->MonsterSay(jessicaQuotes[nextQuote], LANG_UNIVERSAL, 0);

                    nextQuote++;
                    if (nextQuote >= JESSICA_QUOTES_TOTAL)
                        nextQuote = 0;
                }
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

            void Register()
            {
                BeforeHit += SpellHitFn(spell_ring_toss_hit_SpellScript::HitHandler);
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
                    if (shootAura->GetMaxDuration() - shootAura->GetDuration() < 1000)
                    {
                        target->CastSpell(GetCaster(), SPELL_HESHOOTS_KILL_CREDIT_BONUS, true);
                        Player* pl = GetCaster()->ToPlayer();
                        AchievementEntry const* achiev = sAchievementStore.LookupEntry(ACHIEVEMENT_QUICK_SHOT);
                        if (pl)
                            pl->CompletedAchievement(achiev);
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

void AddSC_darkmoon_island()
{
    new npc_maxima_darkmoon();
    new npc_cannonball_bunny();
    new spell_cannon_prep();

    new npc_jessica_darkmoon();
    new spell_ring_toss_enable();
    new spell_ring_toss_trigger();
    new spell_ring_toss_hit();

    new npc_rinling_darkmoon();
    new spell_heshoots_shoot_hit();
    new spell_heshoots_indicator();
}
