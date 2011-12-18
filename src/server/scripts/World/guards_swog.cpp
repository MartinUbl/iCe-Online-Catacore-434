/* SWOG superguard implementation by freghar (Gregory helped!) */

#include "ScriptPCH.h"


typedef std::list< std::pair<uint32,Unit*> > DismountedPair;

struct quotes {
    bool yell;  /* false = say ; true = yell */
    const char *text;
};

static struct quotes respawn_quotes[] = {
    {false, "Hmm, so there is life after death..."},
    {false, "I can do this all day..."},
    {true, "Now I'm really pissed off!"},
    {false, "Eat, pray, respawn."},
};

static struct quotes aggro_quotes[] = {
    {true, "I'm gonna get medieval on your asses!"},
    {true, "I'll blow you a new hole!"},
    {true, "My job is to kick ass, not make small talk!"},
    {false, "My balls, your face."},
    {true, "I'm going to kill you old style!"},
    {false, "You've got a lot of guts. Let's see what they look like."},
    {true, "Come get some!"},
    {true, "Let's rock!"},
};

static struct quotes kill_quotes[] = {
    {false, "Damn, I'm good."},
    {false, "Damn, you're ugly."},
    {true, "Let God sort 'em out!"},
    {false, "My boot, your face; the perfect couple."},
    {true, "You guys suck!"},
    {true, "I'm an equal opportunity asskicker!"},
    {false, "If it bleeds, I can kill it."},
    {false, "Did I promise to kill you last? I lied."},
    {false, "Right in the jewels."},
    {true, "Rest in pieces!"},
};

static struct quotes death_quotes[] = {
    {false, "Shit happens."},
    {false, "Terminated."},
    {false, "Hell, I'd still hit it."},
    {false, "Looks like the sheet has hit the fan."},
    {false, "Death before Disco."},
};

static struct quotes flee_quotes[] = {
    {true, "I ain´t got time to bleed!"},
    {true, "Lucky son of a *beep*!"},
    {true, "Blow it out your knees!"},
    {true, "Damn!"},
};

static struct quotes shoot_quotes[] = {
    {false, "Ooh, that's gotta hurt."},
    {false, "Welcome to \"Cool's-Ville\", Population: Me."},
    {true, "Hey kids you remember i'm a professional, don't try this at home!"},
    {true, "Good, Bad, I'm the guy with the gun!"},
    {true, "My gun's bigger than yours!"},
    {false, "Life is like a box of ammo."},
};

static struct quotes reached_home_quotes[] = {
    {false, "Do, or do not, there is no try."},
    {false, "Mess with the best, die like the rest."},
    {true, "Hail to the king, baby!"},
    {true, "I am king of the world, baby!"},
    {true, "Yeah, piece of cake!"},
    {true, "Heh, heh, heh... what a mess."},
    {true, "I've got balls of steel!"},
};

static bool aiactive(void)
{
    time_t stamp = time(NULL);
    struct tm time_struct = *localtime(&stamp);

    /* active only for 23:00 - 14:59 */
    if (time_struct.tm_hour > 22 || time_struct.tm_hour < 15)
        return true;

    return false;
}

static void doquote(int chance_pct, struct quotes *ptr, int size, Creature *me)
{
    int numquotes;
    int i;

    if (!aiactive())
        return;

    if (urand(0,99) >= uint32(chance_pct))
        return;

    numquotes = size / sizeof(struct quotes);
    i = urand(0, numquotes-1);

    if (ptr[i].yell)
        me->MonsterYell(ptr[i].text, LANG_UNIVERSAL, 0);
    else
        me->MonsterSay(ptr[i].text, LANG_UNIVERSAL, 0);
}

#if 0
static Unit *most_hated_by(Creature *me)
{
    std::list<HostileReference*>& tlist = me->getThreatManager().getThreatList();
    if (tlist.empty())
        return NULL;

    if (Unit *target = Unit::GetUnit(*me, tlist.front()->getUnitGuid()))
        return target;

    return NULL;
}
#endif

/* returns false if there's no nearby creature */
static bool flee_for_assistance(Creature *c, float searchradius)
{
    if (!c->getVictim() || searchradius <= 0)
        return false;

    /* anti-flee (paladin, ...) auras */
    if (c->HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return false;

    Creature *helper = NULL;
    CellPair p(Trinity::ComputeCellPair(c->GetPositionX(), c->GetPositionY()));
    Cell cell(p);
    cell.data.Part.reserved = ALL_DISTRICT;
    cell.SetNoCreate();
    Trinity::NearestAssistCreatureInCreatureRangeCheck u_check(c, c->getVictim(), searchradius);
    Trinity::CreatureLastSearcher<Trinity::NearestAssistCreatureInCreatureRangeCheck> searcher(c, helper, u_check);

    TypeContainerVisitor<Trinity::CreatureLastSearcher<Trinity::NearestAssistCreatureInCreatureRangeCheck>, GridTypeMapContainer > grid_creature_searcher(searcher);
    cell.Visit(p, grid_creature_searcher, *c->GetMap(), *c, searchradius);

    c->SetNoSearchAssistance(true);
    c->UpdateSpeed(MOVE_RUN, false);

    if (!helper)
        return false;

    /* caugth a flying guard, probably */
    if (fabs(c->GetPositionZ() - helper->GetPositionZ()) > 40.0f)
        return false;

    c->GetMotionMaster()->Clear();
    c->GetMotionMaster()->MoveSeekAssistance(helper->GetPositionX(), helper->GetPositionY(), helper->GetPositionZ());

    return true;
}


struct guards_swogAI : public ScriptedAI
{
    guards_swogAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    ~guards_swogAI() {}

    int lowhp_timer;
    int lowhp_percent;
    DismountedPair dismounted;

    void Reset() {
        lowhp_timer = 60000;
        lowhp_percent = urand(15,40);

        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, false);
        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, false);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
    }

    void EnterCombat(Unit *who) {
        if (!aiactive())
            return;

        /* 1% chance of "death grip"-like pull */
        if (urand(0,99) < 1 && !who->IsMounted() && !who->IsMountedShape()) {
            me->CastSpell(who, 49576, true);
            me->MonsterYell("Come to meet your maker!", LANG_UNIVERSAL, 0);
        } else {
            doquote(10, aggro_quotes, sizeof(aggro_quotes), me);
        }
    }

    void KilledUnit(Unit *who) {
        doquote(30, kill_quotes, sizeof(kill_quotes), me);
    }

    void JustDied(Unit *who) {
        doquote(10, death_quotes, sizeof(death_quotes), me);
        Reset();

        if (Player* pKiller = who->GetCharmerOrOwnerPlayerOrPlayerItself()) {
            me->SendZoneUnderAttackMessage(pKiller);
            if (aiactive())
                pKiller->RewardHonor(NULL,1,1);  /* add symbolic 1 honor */
        }
    }

    /*virtual*/ void JustRespawned() {
        doquote(20, respawn_quotes, sizeof(respawn_quotes), me);
        Reset();
    }

    /*virtual*/ void JustReachedHome() {
        doquote(10, reached_home_quotes, sizeof(reached_home_quotes), me);
        Reset();
    }

    void ReceiveEmote(Player* who, uint32 emote) {
        if (me->isInCombat())
            return;
        if (emote == TEXTEMOTE_SALUTE && me->IsWithinDistInMap(who, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY))) {
            std::ostringstream report;
            report << "SWOGAI  ";
            report << me->GetGUIDLow();
            report << (aiactive() ? "  reporting to duty, " : "  in sleep mode, ");
            report << ((who->getGender() == GENDER_MALE) ? "sir!" : "madame!");
            me->MonsterSay(report.str().c_str(), LANG_UNIVERSAL, 0);
            me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
        }
    }

    void MoveInLineOfSight(Unit* who);
    void UpdateAI(const uint32 diff);
};


void guards_swogAI::MoveInLineOfSight(Unit* who)
{
    if (who->GetTypeId() != TYPEID_PLAYER || !me->IsHostileTo(who))
        return;

    if (!me->IsWithinDistInMap(who, VISIBLE_RANGE) || !me->IsWithinLOSInMap(who))
        return;

    //if (me->IsWithinDistInMap(who, 40.0f) /*&& me->canStartAttack(who, false)*/)

    /* limit aggro range to usual value, so what - at least
     * flee for assistance has some meaning */
    if (me->canStartAttack(who, false))
        AttackStart(who);

    /* dismount a mounted unit in visible range */
    if (who->IsMounted() || who->IsMountedShape()) {

        /* was the target already shot? */
        for (DismountedPair::iterator itr = dismounted.begin(); itr != dismounted.end(); ++itr)
            if (itr->second == who)
                return;

        me->CastSpell(who, 18395, false);  /* Dismounting Shot */
        dismounted.push_back(std::make_pair(0, who));

        doquote(10, shoot_quotes, sizeof(shoot_quotes), me);
    }
}

void guards_swogAI::UpdateAI(const uint32 diff)
{
    /* delete dismounted entries older than 10secs
     * (needs to be before UpdateVictim as it can be performed OOC) */
    for (DismountedPair::iterator itr = dismounted.begin(); itr != dismounted.end(); ++itr)
        itr->first += diff;
    while (!dismounted.empty() && dismounted.front().first >= 10000)
        dismounted.pop_front();

    if (!UpdateVictim())
        return;

    /* combat timers */
    lowhp_timer += diff;

    /* do nothing while fleeing for assistance */
    switch (me->GetMotionMaster()->GetCurrentMovementGeneratorType()) {
        case ASSISTANCE_MOTION_TYPE:
        case ASSISTANCE_DISTRACT_MOTION_TYPE:
        case TIMED_FLEEING_MOTION_TYPE:
        case FLEEING_MOTION_TYPE:
            return;
        default:
            break;
    }

    /* do one of several possible actions on low health
     * but only once each 60 seconds (== ie. potion cooldown) */
    if (me->GetHealthPct() < lowhp_percent && lowhp_timer >= 60000 && aiactive()) {
        lowhp_timer = 0;
        switch (urand(0,5)) {

            /* first possible action: look for assistance, if none found, despawn */
            case 0:
            case 1:  /* twice the chance */
                /* look around 60yd for assistance */
                if (flee_for_assistance(me, 60.0f)) {
                    if (urand(0,1)) {
                        me->CastSpell(me, 78989, false);  /* potion, 21000 hp */
                    } else {
                        const int32 bp = 21000;
                        me->CastCustomSpell(me, 10577, &bp, 0, 0, true);  /* spell, 21000 hp */
                    }
                    doquote(20, flee_quotes, sizeof(flee_quotes), me);
                } else {
                    /* no helper found, run/teleport away */
                    if (me->getFaction() == 11) {
                        /* alliance: simulate paladin - divine shield + HS */
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                        me->CastSpell(me, 40733, true);  /* infinite divine shield */
                        me->CastSpell(me, 8690, false);  /* normal hearthstone */
                        me->ForcedDespawn(10300);  /* 10sec = HS cast time */
                    } else {
                        /* horde/other: simulate a coward - flee away (very fast) */
                        me->RemoveAllAuras();
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, true);
                        me->ApplySpellImmune(0, IMMUNITY_DAMAGE, SPELL_SCHOOL_MASK_MAGIC, true);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        me->CastSpell(me, 66179, true);  /* visual + speed (but insufficient) */
                        me->SetSpeed(MOVE_RUN, 3.0f);
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveFleeing(me->getVictim(), 0);
                        me->ForcedDespawn(2000);  /* ~2sec = ~visible range  */
                    }
                    doquote(20, flee_quotes, sizeof(flee_quotes), me);
                }
                break;

            /* third possible action: boost (full hp / superhaste) */
            case 2:
                if (urand(0,1)) {
                    me->CastSpell(me, 64460, true);  /* instant full hp + visual */
                } else {
                    const int32 bp = 200;
                    me->CastCustomSpell(me, 57060, &bp, 0, 0, true);  /* +200% attack speed */
                }
                doquote(10, aggro_quotes, sizeof(aggro_quotes), me);
                break;

            /* fourth possible action: supress current victim (dispellable) */
            case 3:
                /* if top victim is spellcaster, use silence, else cripple */
                if (me->getVictim()->GetMaxPower(POWER_MANA)) {
                    SpellEntry *wrentry = sSpellStore.LookupEntryNoConst(38913);
                    if (!wrentry)
                        break;
                    /* not my idea! don't flame me for this blasphemy! */
                    uint32 oldindex = wrentry->DurationIndex;
                    wrentry->DurationIndex = 1;
                    me->CastSpell(me->getVictim(), wrentry, true);  /* 10sec silence */
                    wrentry->DurationIndex = oldindex;
                } else {
                    me->CastSpell(me->getVictim(), 41281, true);  /* triggered (instant) cripple */
                }
                break;

            /* fifth possible action: nasty AoE */
            case 4:
                if (me->getFaction() == 11) {
                    /* alliance: simulate paladin - consecration */
                    const int32 bp = 10000;
                    me->CastCustomSpell(me, 36946, &bp, 0, 0, true);  /* 10000 dmg / sec, 10yd, 20sec */

                } else {
                    /* horde/other: simulate warlock - rain of fire */
                    const int32 bp = 15000;
                    me->CastCustomSpell(me, 31340, &bp, 0, 0, true);  /* 15000 dmg / 2sec, 15yd, 10sec */
                }
                break;

            /* sixth possible action: call all same-entry npcs to assist "me" */
            case 5:
                {
                    std::list<Creature*> friends;
                    me->GetCreatureListWithEntryInGrid(friends, me->GetEntry(), 40.0f);
                    if (friends.empty())
                        break;

                    for (std::list<Creature*>::const_iterator cfriend = friends.begin(); cfriend != friends.end(); ++cfriend)
                        if (!(*cfriend)->isInCombat())
                            (*cfriend)->AI()->AttackStart(me->getVictim());
                }

                me->getVictim()->CastSpell(me->getVictim(), 59908, true);  /* nice ebon blade mark, selfcast! */
                doquote(10, aggro_quotes, sizeof(aggro_quotes), me);
                break;

            /* seventh possible action: do nothing */
            /* DISABLED: occurs on sixth possible action
             *           when there's no friend found
            case 6:
                break; */

            default:
                break;
        }
    }

    DoMeleeAttackIfReady();
}


class guards_swog : public CreatureScript
{
public:
    guards_swog() : CreatureScript("guards_swog") { }

    CreatureAI *GetAI(Creature *creature) const
    {
        return new guards_swogAI(creature);
    }
};

void AddSC_guards_swog()
{
    new guards_swog;
}

