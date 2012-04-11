/*
 * Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ScriptPCH.h"
#include "BattlegroundAB.h"
#include "BattlegroundWS.h"
#include "BattlegroundIC.h"
#include "BattlegroundSA.h"

class achievement_school_of_hard_knocks : public AchievementCriteriaScript
{
    public:
        achievement_school_of_hard_knocks() : AchievementCriteriaScript("achievement_school_of_hard_knocks") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            static uint32 const orphanEntries[6] = {14305, 14444, 22818, 22817, 33533, 33532};
            uint32 currentPet = GUID_ENPART(source->GetCritterGUID());
            for (uint8 i = 0; i < 6; ++i)
                if (currentPet == orphanEntries[i])
                    return true;

            return false;
        }
};

class achievement_storm_glory : public AchievementCriteriaScript
{
    public:
        achievement_storm_glory() : AchievementCriteriaScript("achievement_storm_glory") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (source->GetBattlegroundTypeId() != BATTLEGROUND_EY)
                return false;

            Battleground *pEotS = source->GetBattleground();
            if (!pEotS)
                return false;

            return pEotS->IsAllNodesConrolledByTeam(source->GetTeam());
        }
};

class achievement_resilient_victory : public AchievementCriteriaScript
{
    public:
        achievement_resilient_victory() : AchievementCriteriaScript("achievement_resilient_victory") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            Battleground* bg = source->GetBattleground();
            if (!bg)
                return false;

            if (bg->GetTypeID(true) != BATTLEGROUND_AB)
                return false;

            if (!static_cast<BattlegroundAB*>(bg)->IsTeamScores500Disadvantage(source->GetTeam()))
                return false;

            return true;
        }
};

class achievement_bg_control_all_nodes : public AchievementCriteriaScript
{
    public:
        achievement_bg_control_all_nodes() : AchievementCriteriaScript("achievement_bg_control_all_nodes") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            Battleground* bg = source->GetBattleground();
            if (!bg)
                return false;

            if (!bg->IsAllNodesConrolledByTeam(source->GetTeam()))
                return false;

            return true;
        }
};

class achievement_save_the_day : public AchievementCriteriaScript
{
    public:
        achievement_save_the_day() : AchievementCriteriaScript("achievement_save_the_day") { }

        bool OnCheck(Player* source, Unit* target)
        {
            if (!target)
                return false;

            if (Player const* pTarget = target->ToPlayer())
            {
                if (source->GetBattlegroundTypeId() != BATTLEGROUND_WS || !source->GetBattleground())
                    return false;

                BattlegroundWS* pWSG = static_cast<BattlegroundWS*>(source->GetBattleground());
                if (pWSG->GetFlagState(pTarget->GetTeam()) == BG_WS_FLAG_STATE_ON_BASE)
                    return true;
            }
            return false;
        }
};

class achievement_bg_ic_resource_glut : public AchievementCriteriaScript
{
    public:
        achievement_bg_ic_resource_glut() : AchievementCriteriaScript("achievement_bg_ic_resource_glut") { }

        bool OnCheck(Player* source, Unit* /*target*/)
        {
            if (source->HasAura(SPELL_OIL_REFINERY) && source->HasAura(SPELL_QUARRY))
                return true;

            return false;
        }
};

class achievement_bg_ic_glaive_grave : public AchievementCriteriaScript
{
    public:
        achievement_bg_ic_glaive_grave() : AchievementCriteriaScript("achievement_bg_ic_glaive_grave") { }

        bool OnCheck(Player* source, Unit* target)
        {
            if (Creature* vehicle = source->GetVehicleCreatureBase())
            {
                if (vehicle->GetEntry() == 35273 || vehicle->GetEntry() == 34802)
                    return true;
            }

            return false;
        }
};

class achievement_bg_ic_mowed_down : public AchievementCriteriaScript
{
    public:
        achievement_bg_ic_mowed_down() : AchievementCriteriaScript("achievement_bg_ic_mowed_down") { }

        bool OnCheck(Player* source, Unit* target)
        {
            if (Creature* vehicle = source->GetVehicleCreatureBase())
            {
                if (vehicle->GetEntry() == NPC_KEEP_CANNON)
                    return true;
            }

            return false;
        }
};

//class achievement_bg_sa_artillery_veteran : public AchievementCriteriaScript
//{
//    public:
//        achievement_bg_sa_artillery_veteran() : AchievementCriteriaScript("achievement_bg_sa_artillery_veteran") { }
//
//        bool OnCheck(Player* source, Unit* target)
//        {
//            if (Creature* vehicle = source->GetVehicleCreatureBase())
//            {
//                if (vehicle->GetEntry() == NPC_ANTI_PERSONNAL_CANNON)
//                    return true;
//            }
//
//            return false;
//        }
//};
//
//class achievement_bg_sa_artillery_expert : public AchievementCriteriaScript
//{
//    public:
//        achievement_bg_sa_artillery_expert() : AchievementCriteriaScript("achievement_bg_sa_artillery_expert") { }
//
//        bool OnCheck(Player* source, Unit* target)
//        {
//            if (Creature* vehicle = source->GetVehicleCreatureBase())
//            {
//                if (vehicle->GetEntry() != NPC_ANTI_PERSONNAL_CANNON)
//                    return false;
//
//                BattlegroundSA* SA = static_cast<BattlegroundSA*>(source->GetBattleground());
//                return SA->GetPlayerDemolisherScore(source);
//            }
//
//            return false;
//        }
//};
//
//class achievement_bg_sa_drop_it : public AchievementCriteriaScript
//{
//    public:
//        achievement_bg_sa_drop_it() : AchievementCriteriaScript("achievement_bg_sa_drop_it") { }
//
//        enum AchievementData
//        {
//            SPELL_CARRYING_SEAFORIUM = 52418,            
//        };
//
//        bool OnCheck(Player* /*source*/, Unit* target)
//        {
//            if (target->HasAura(SPELL_CARRYING_SEAFORIUM))
//                return true;
//
//            return false;
//        }
//};

class go_romantic_picnic: public GameObjectScript
{
public:
    go_romantic_picnic(): GameObjectScript("go_romantic_picnic") {}

    bool OnGossipHello(Player* pPlayer, GameObject* pObject)
    {
        pPlayer->CastSpell(pPlayer,45123,true);
        pPlayer->SetStandState(UNIT_STAND_STATE_SIT);

        if(Unit* pOwner = pObject->GetOwner())
        {
            Player* pPlayerOwner = (Player*)pOwner;
            if(pPlayerOwner != pPlayer && pPlayer->HasAura(71074) && pPlayerOwner->HasAura(71074))
            {
                const AchievementEntry* achiev = sAchievementStore.LookupEntry(1291);
                if(achiev)
                {
                    pPlayerOwner->CompletedAchievement(achiev);
                    pPlayer->CompletedAchievement(achiev);
                }
            }
        }
        else
            return false;

        return true;
    }
};

class npc_love_fool: public CreatureScript
{
public:
    npc_love_fool(): CreatureScript("npc_love_fool") {}

    enum
    {
        CRITERIA_WINTERGRASP             = 6343,
        CRITERIA_GURUBASHI_ARENA         = 6344,
        CRITERIA_ARATHI_BASIN_BLACKSMITH = 6345,
        CRITERIA_CULLING_OF_STRATHOLME   = 6346,
        CRITERIA_NAXXRAMAS               = 6347
    };

    struct love_foolAI: public PassiveAI
    {
        love_foolAI(Creature* c): PassiveAI(c) { }

        void ReceiveEmote(Player* pPlayer, uint32 text_emote)
        {
            if(text_emote == TEXTEMOTE_PITY)
            {
                if(pPlayer->GetAreaId() == 3421)
                    pPlayer->GetAchievementMgr().SetCriteriaProgress(CRITERIA_ARATHI_BASIN_BLACKSMITH, 1704, 1, PROGRESS_SET);
                if(pPlayer->GetZoneId() == 4197)
                    pPlayer->GetAchievementMgr().SetCriteriaProgress(CRITERIA_WINTERGRASP, 1704, 1, PROGRESS_SET);
                if(pPlayer->GetMapId() == 595)
                    pPlayer->GetAchievementMgr().SetCriteriaProgress(CRITERIA_CULLING_OF_STRATHOLME, 1704, 1, PROGRESS_SET);
                if(pPlayer->GetAreaId() == 2177)
                    pPlayer->GetAchievementMgr().SetCriteriaProgress(CRITERIA_GURUBASHI_ARENA, 1704, 1, PROGRESS_SET);
                if(pPlayer->GetMapId() == 533)
                    pPlayer->GetAchievementMgr().SetCriteriaProgress(CRITERIA_NAXXRAMAS, 1704, 1, PROGRESS_SET);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new love_foolAI(pCreature);
    }
};

const uint32 PerfumeIds[] = {70234, 70235, 70233, 71507, 26682};

class npc_sraaz_or_jeremiah: public CreatureScript
{
public:
    npc_sraaz_or_jeremiah(): CreatureScript("npc_sraaz_or_jeremiah") {}

    enum
    {
        CRITERIA_KISS_SRAAZ = 3931,
        CRITERIA_KISS_JEREMIAH = 3929,
        CRITERIA_HANDFUL_ON_SRAAZ = 12859,
        CRITERIA_HANDFUL_ON_JEREMIAH = 4227,
        ACHIEVEMENT_FLIRT_WITH_DISASTER_A = 1279,
        ACHIEVEMENT_FLIRT_WITH_DISASTER_H = 1280
    };

    struct sharedAI: public ScriptedAI
    {
        sharedAI(Creature* c): ScriptedAI(c)
        {
            if (me->GetEntry() == 8403) //Jeremiah
            {
                CriteriaKiss = CRITERIA_KISS_JEREMIAH;
                CriteriaHandful = CRITERIA_HANDFUL_ON_JEREMIAH;
                AchievementId = ACHIEVEMENT_FLIRT_WITH_DISASTER_H;
            }
            else if (me->GetEntry() == 9099) //Sraaz
            {
                CriteriaKiss = CRITERIA_KISS_SRAAZ;
                CriteriaHandful = CRITERIA_HANDFUL_ON_SRAAZ;
                AchievementId = ACHIEVEMENT_FLIRT_WITH_DISASTER_A;
            }
        }

        uint32 CriteriaKiss;
        uint32 CriteriaHandful;
        uint32 AchievementId;

        bool HavePerfume(Player* pPlayer)
        {
            for (uint32 i = 0; i < uint32(sizeof(PerfumeIds)/sizeof(uint32)); i++)
            {
                if (pPlayer->HasAura(PerfumeIds[i]))
                    return true;
            }
            return false;
        }

        void ReceiveEmote(Player* pPlayer, uint32 text_emote)
        {
            if (pPlayer->GetDrunkValue() < 0xFFF || !HavePerfume(pPlayer))
                return;

            if (text_emote == TEXTEMOTE_KISS)
            {
                pPlayer->GetAchievementMgr().SetCriteriaProgress(CriteriaKiss, AchievementId, 1, PROGRESS_SET);
            }
        }

        void SpellHit(Unit* caster, const SpellEntry* spell)
        {
            Player* pPlayer = (Player*)caster;

            if (!pPlayer || pPlayer->GetDrunkValue() < 0xFFF || !HavePerfume(pPlayer))
                return;

            if (spell->Id == 27571)
                pPlayer->GetAchievementMgr().SetCriteriaProgress(CriteriaHandful, AchievementId, 1, PROGRESS_SET);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new sharedAI(pCreature);
    }
};

class achievement_bunny_maker : public AchievementCriteriaScript
{
    public:
        achievement_bunny_maker() : AchievementCriteriaScript("achievement_bunny_maker") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return false;
                
            if (target->getLevel() >= 18 && target->getGender() == GENDER_FEMALE)
                return true;
            else
                return false;
        }
};

void AddSC_achievement_scripts()
{
    new achievement_school_of_hard_knocks();
    new achievement_storm_glory();
    new achievement_resilient_victory();
    new achievement_bg_control_all_nodes();
    new achievement_save_the_day();
    new achievement_bg_ic_resource_glut();
    new achievement_bg_ic_glaive_grave();
    new achievement_bg_ic_mowed_down();
//    new achievement_bg_sa_artillery_veteran();
//    new achievement_bg_sa_artillery_expert();
//    new achievement_bg_sa_drop_it();
    new go_romantic_picnic();
    new npc_sraaz_or_jeremiah();
    new npc_love_fool();
    new achievement_bunny_maker();
}
