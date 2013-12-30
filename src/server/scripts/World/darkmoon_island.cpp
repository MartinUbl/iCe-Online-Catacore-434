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
#define MAXIMA_GOSSIP_1_LAUNCH "Launch me! (Darkmoon Game Token)"
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

void AddSC_darkmoon_island()
{
    new npc_maxima_darkmoon();
    new npc_cannonball_bunny();
    new spell_cannon_prep();
}
