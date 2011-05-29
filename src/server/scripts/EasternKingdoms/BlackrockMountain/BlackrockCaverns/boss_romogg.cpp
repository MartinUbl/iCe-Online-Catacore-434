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
#include "blackrock_caverns.h"

/*
SQL:
REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`,
`faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`,
`trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`,
`spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`,
`questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (39665, 0, 0, 0, 0, 0, 33147, 0, 0, 0, 'Rom\'ogg Bonecrusher', '', '', 0, 85, 85, 0, 14, 14, 0, 1, 1,14286, 1, 1, 163, 210, 0, 932, 15,
2000, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 104, 39665, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19004, 21004, '', 0, 3, 146,01, 0, 1, 0, 0, 0, 0, 0, 0, 0, 167, 1, 0, 0, 0, 'boss_romogg', 1);

REPLACE INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`,
`faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`,
`trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`,
`spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`,
`questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`) VALUES (40447, 0, 0, 0, 0, 0, 1126, 33553, 0, 0, 'Chains of Woe', '', '', 0, 80, 84, 0, 14, 14, 0, 1, 1.14286, 1, 1, 165, 213, 0, 945, 15, 2000,
0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 3, 7.573, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 'mob_chains_of_woe', 1);
*/

enum
{
    SPELL_CALL_FOR_HELP      = 82137,
    SPELL_QUAKE              = 75272,
    SPELL_CHAINS_OF_WOE      = 75539,
    SPELL_SKULLCRACKER       = 75543,
    SPELL_SKULLCRACKER_H     = 93453,
    SPELL_WOUNDING_STRIKE    = 75571,
    SPELL_WOUNDING_STRIKE_H  = 93452,

    SPELL_CHAINS_OF_WOE_AURA      = 75441,
    SPELL_CHAINS_OF_WOE_TRIGGERED = 82189,
    SPELL_CHAINS_OF_WOE_STUN      = 82192,
    SPELL_CHAINS_OF_WOE_TELEPORT  = 75464,

    MOB_ANGERED_EARTH        = 50376,
    MOB_CHAINS_OF_WOE        = 40447,
};

class boss_romogg: public CreatureScript
{
public:
    boss_romogg(): CreatureScript("boss_romogg") {}

    struct romoggAI: public ScriptedAI
    {
        romoggAI(Creature* pCreature): ScriptedAI(pCreature)
        {
            ModifySpellRadius(75428, 32, 0);
            ModifySpellRadius(75272, 32, 0);
            pInstance = me->GetInstanceScript();
            Reset();
        }

        void Reset()
        {
            WoeCast = 0;
            SkullcrackTimer = 0;
            ElementalsTimer = 0;

            QuakeTimer = 9000;
            StrikeTimer = 7000;
        }

        InstanceScript* pInstance;

        uint32 QuakeTimer;
        uint32 StrikeTimer;
        uint32 WoeCast;
        uint32 SkullcrackTimer;
        uint32 ElementalsTimer;

        void EnterCombat(Unit* pWho)
        {
            me->MonsterYell("Boss Cho'gall not gonna be happy 'bout dis!",LANG_UNIVERSAL,0);
            me->CastSpell(me,SPELL_CALL_FOR_HELP,false);
        }

        void KilledUnit(Unit* pVictim)
        {
            me->MonsterYell("That what you get! Nothing!",LANG_UNIVERSAL,0);
        }

        void JustDied(Unit* pKiller)
        {
            me->MonsterYell("Rom'ogg...sorry...",LANG_UNIVERSAL,0);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->GetHealthPct() <= 66.6f && WoeCast == 0)
            {
                WoeCast = 1;
                me->CastSpell(me,SPELL_CHAINS_OF_WOE, false);
                SkullcrackTimer = 3000;
            }
            if (me->GetHealthPct() <= 33.3f && WoeCast == 1)
            {
                WoeCast = 2;
                me->CastSpell(me,SPELL_CHAINS_OF_WOE, false);
                SkullcrackTimer = 3000;
            }

            if (SkullcrackTimer)
            {
                if (SkullcrackTimer <= diff)
                {
                    me->MonsterYell("Stand still! Rom'ogg crack your skulls!",LANG_UNIVERSAL,0);
                    me->CastSpell(me, SPELL_SKULLCRACKER, false);
                    SkullcrackTimer = 0;
                } else SkullcrackTimer -= diff;
            }

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            if (QuakeTimer <= diff)
            {
                me->CastSpell(me, SPELL_QUAKE, false);
                QuakeTimer = 12000;
                ElementalsTimer = 3000;
            } else QuakeTimer -= diff;

            if (StrikeTimer <= diff)
            {
                me->CastSpell(me->getVictim(), SPELL_WOUNDING_STRIKE, false);
                StrikeTimer = 15000;
            } else StrikeTimer -= diff;

            if (ElementalsTimer && pInstance)
            {
                if (ElementalsTimer <= diff)
                {
                    Map::PlayerList const& plList = pInstance->instance->GetPlayers();
                    if (!plList.isEmpty())
                    {
                        Position pos;
                        for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                        {
                            itr->getSource()->GetPosition(&pos);
                            Creature* pSpirit = me->SummonCreature(MOB_ANGERED_EARTH,pos,TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT,30000);
                            if (pSpirit)
                                pSpirit->AddThreat(itr->getSource(),1.0f);
                        }
                    }
                    ElementalsTimer = 0;
                } else ElementalsTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new romoggAI(c);
    }
};

class mob_chains_of_woe: public CreatureScript
{
public:
    mob_chains_of_woe(): CreatureScript("mob_chains_of_woe") {};

    struct chainsAI: public Scripted_NoMovementAI
    {
        chainsAI(Creature* c): Scripted_NoMovementAI(c)
        {
            //Invisible displayid
            me->SetDisplayId(16946);
            //Chains of woe visual radius
            ModifySpellRadius(82189, 11, 0);
            pInstance = me->GetInstanceScript();
            Reset();
        }

        InstanceScript* pInstance;

        void Reset()
        {
            me->CastSpell(me, SPELL_CHAINS_OF_WOE_AURA, true);

            if (!pInstance)
                return;

            //me->CastSpell(me, SPELL_CHAINS_OF_WOE_TELEPORT, true);
            // Workaround
            Map::PlayerList const& plList = pInstance->instance->GetPlayers();
            if (!plList.isEmpty())
            {
                float x,y,z;
                for(Map::PlayerList::const_iterator itr = plList.begin(); itr != plList.end(); ++itr)
                {
                    me->GetNearPoint2D(x,y,2.0f,urand(0,6.28f));
                    z = itr->getSource()->GetPositionZ();
                    itr->getSource()->NearTeleportTo(x,y,z,itr->getSource()->GetOrientation());
                }
            }
        }

        void SpellHitTarget(Unit* pTarget, const SpellEntry* spell)
        {
            if (spell->Id == SPELL_CHAINS_OF_WOE_TRIGGERED)
            {
                if (pTarget->HasAura(SPELL_CHAINS_OF_WOE_STUN))
                    pTarget->GetAura(SPELL_CHAINS_OF_WOE_STUN)->RefreshDuration();
                else
                    pTarget->CastSpell(pTarget, SPELL_CHAINS_OF_WOE_STUN, true);
            }
        }
    };

    CreatureAI* GetAI(Creature* c) const
    {
        return new chainsAI(c);
    }
};

void AddSC_romogg()
{
    new boss_romogg();
    new mob_chains_of_woe();
}

