/*
 *  Baradin Hold boss Occu'thar
 *  added in 4.2.0
 *
 *  abilities:
 *  Berserk             100%
 *  Searing Shadows     100%
 *  Focused Fire        100%
 *  Eyes of Occu'thar   100%
 *
 *  Blizz-like AI by OJaaaa, 2013
 *
 */

#include"ScriptPCH.h"
#include"baradin_hold.h"

enum Spells
{
    SPELL_BERSERK               = 47008,

    SPELL_SEARING_SHADOWS       = 96913,

    NPC_EYESTALKER              = 52368, // occuthar eye
    NPC_EYEBEAM                 = 52369, // on target location
    SPELL_FOCUSED_FIRE_SUMMON   = 96872,
    SPELL_FOCUSED_FIRE_AOE      = 96884,
    SPELL_FOCUSED_FIRE_BEAM     = 96886,
    SPELL_FOCUSED_FIRE_VISUAL   = 97213,
    SPELL_FOCUSED_FIRE_ONESHOT  = 97212,

    NPC_EYE_OF_OCCUTHAR         = 52389,
    SPELL_EYES_OF_OCCUTHAR      = 96920,    // boss ability
    SPELL_EYES_OF_OCCUTHAR_H    = 101006,
    SPELL_EYES_OF_OCCUTHAR_VISUAL = 96932,  // also sets vehicle kit to a player
    SPELL_GAZE_OF_OCCUTHAR      = 97028,    // periodically triggers damage spell
    SPELL_GAZE_OF_OCCUTHAR_VISUAL = 96995,  // visual effect
  //SPELL_OCCUTHARS_DESTRUCTION = 96968,    // AoE used in HandleAuraSpecificMods
};

class boss_occuthar: public CreatureScript
{
public:
    boss_occuthar() : CreatureScript("boss_occuthar") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
       return new boss_occutharAI(pCreature);
    }
    struct boss_occutharAI: public ScriptedAI
    {
        boss_occutharAI(Creature* pCreature) : ScriptedAI(pCreature), Summons(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();
            vehicle = me->GetVehicleKit();
            if (vehicle)
                seatcnt = vehicle->m_Seats.size();

            pCreature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            pCreature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 49560, true); // Death Grip jump effect
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 81261, true); // Solar Beam
            pCreature->ApplySpellImmune(0, IMMUNITY_ID, 88625, true); // Chastise
        }

        InstanceScript *pInstance;
        Vehicle *vehicle;
        int8 seatcnt;

        bool berserk;
        uint32 count;
        uint32 berserk_timer;
        uint32 shadows_timer;
        uint32 eyes_timer;

        SummonList Summons; // the eyes on Occu'thars head are stored here

        void Reset()
        {
            berserk = false;
            count = 0;
            berserk_timer = 300000; // 5 min enrage timer
            shadows_timer = 9000;
            eyes_timer = 13000;

            Summons.DespawnAll();

            for (int8 i = 0; i < seatcnt; i++)
            {
                if (Creature *summon = me->SummonCreature(NPC_EYESTALKER, 0, 0, 0))
                {
                    Summons.Summon(summon);
                    if (vehicle)
                        summon->EnterVehicle(vehicle, i);
                }
            }

            if (pInstance && pInstance->GetData(DATA_OCCUTHAR_EVENT) != DONE)
                pInstance->SetData(DATA_OCCUTHAR_EVENT, NOT_STARTED);
        }

        void EnterCombat(Unit* /*Ent*/)
        {
            DoZoneInCombat(me);
            if (pInstance)
                pInstance->SetData(DATA_OCCUTHAR_EVENT, IN_PROGRESS);
        }

        void JustDied(Unit* /*Kill*/)
        {
            if (pInstance)
                pInstance->SetData(DATA_OCCUTHAR_EVENT, DONE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            // pure butchery
            if (berserk)
            {
                DoMeleeAttackIfReady();
                return;
            }

            // Berserk timer
            if (berserk_timer < diff)
            {
                berserk = true;
                me->CastSpell(me, SPELL_BERSERK, true);
            } else berserk_timer -= diff;

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            // Searing Shadows timer
            if (shadows_timer < diff)
            {
                me->CastSpell(me, SPELL_SEARING_SHADOWS, false);
                shadows_timer = 19000;
            } else shadows_timer -= diff;

            // Focused Fire / Eyes of Occu'thar (not on tank)
            if (eyes_timer < diff)
            {
                count++;
                if (count%3)
                {
                    // Focused Fire
                    if (Unit *target = this->SelectUnit(SELECT_TARGET_RANDOM,1))
                        me->CastSpell(target, SPELL_FOCUSED_FIRE_SUMMON, false);
                }
                else
                {
                    // Eyes of Occu'thar
                    me->CastSpell(me, SPELL_EYES_OF_OCCUTHAR, false);
                }
                eyes_timer = 14000;
            } else eyes_timer -= diff;

            // White damage
            DoMeleeAttackIfReady();
        }

        void JustSummoned(Creature* summon)
        {
            if (!summon)
                return;

            switch (summon->GetEntry())
            {
            case NPC_EYEBEAM:
                {
                    summon->CastSpell(summon, SPELL_FOCUSED_FIRE_VISUAL, true);
                    me->CastSpell(summon, SPELL_FOCUSED_FIRE_ONESHOT, true);
                    me->CastSpell(summon, SPELL_FOCUSED_FIRE_AOE, true);

                    for(SummonList::const_iterator itr = Summons.begin(); itr != Summons.end(); ++itr)
                    {
                        if(Unit* eye = me->GetCreature(*me, *itr))
                        {
                            eye->CastSpell(summon, SPELL_FOCUSED_FIRE_BEAM, true);
                        }
                    }

                    break;
                }
            default: break;
            }
        }

        void SpellHitTarget(Unit *target, const SpellEntry *spell)
        {
            if (!spell || !target)
                return;

            switch (spell->Id)
            {
            case SPELL_EYES_OF_OCCUTHAR:
            case SPELL_EYES_OF_OCCUTHAR_H:
                {
                    if (target == me->getVictim() || target->GetTypeId() != TYPEID_PLAYER)
                        break;
                    me->CastSpell(target, SPELL_EYES_OF_OCCUTHAR_VISUAL, true);
                    break;
                }
            case SPELL_EYES_OF_OCCUTHAR_VISUAL: // also creates vehicle kit on player for 10 sec
                {
                    if (Aura *aura = target->GetAura(SPELL_EYES_OF_OCCUTHAR_VISUAL)) 
                    {
                        aura->SetDuration(15000); // prevent vehicle kit aura from fading prematurely
                        if (Unit *eye = me->SummonCreature(NPC_EYE_OF_OCCUTHAR, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0))
                        {
                            eye->EnterVehicle(target,0);
                            eye->clearUnitState(UNIT_STAT_UNATTACKABLE); // applied when entering vehicle
                            eye->CastSpell(target, SPELL_GAZE_OF_OCCUTHAR, true);
                            eye->CastSpell(eye, SPELL_GAZE_OF_OCCUTHAR_VISUAL, true);
                        }
                    }
                    break;
                }
            default: break;
            }
        }

     };
};


class mob_occuthar_eyestalk: public CreatureScript
{
public:
    mob_occuthar_eyestalk() : CreatureScript("mob_occuthar_eyestalk") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
       return new mob_occuthar_eyestalkAI(pCreature);
    }
    struct mob_occuthar_eyestalkAI: public Scripted_NoMovementAI
    {
        mob_occuthar_eyestalkAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            pCreature->SetReactState(REACT_PASSIVE);
            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            //pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void InitializeAI() { Scripted_NoMovementAI::InitializeAI(); }

        void Reset() { }

        void EnterCombat(Unit* /*pWho*/) { }
        void DamageTaken(Unit* /*pDoneBy*/, uint32 &damage) { damage = 0; }
        void UpdateAI(const uint32 diff) { }
    };
};


/* SQL
UPDATE `creature_template` SET ScriptName='boss_occuthar', VehicleId=1598, difficulty_entry_1=54229 WHERE entry=52363;
UPDATE `creature_template` SET VehicleId=1598 WHERE entry=54229;

DELETE FROM `creature_template` WHERE entry IN (52369,52368);
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`)
VALUES
('52369','0','0','0','0','0','27823','0','0','0','Eyestalk','Occuthar encounter','','0','87','87','0','14','14','0','0.714286','2','1','0','0','0','0','0','1','2000','2000','1','33554432','0','0','0','0','0','0','0','0','0','10','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','94','1','0','0','0','mob_occuthar_eyestalk','15595'),
('52368','0','0','0','0','0','27823','0','0','0','Eyestalk','Occuthar encounter','','0','1','1','0','14','14','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1','1','1','0','0','0','0','0','0','0','94','1','0','0','0','mob_occuthar_eyestalk','15595');

DELETE FROM `creature_template` WHERE entry IN (52389,52428);
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction_A`, `faction_H`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `mindmg`, `maxdmg`, `dmgschool`, `attackpower`, `dmg_multiplier`, `baseattacktime`, `rangeattacktime`, `unit_class`, `unit_flags`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `minrangedmg`, `maxrangedmg`, `rangedattackpower`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `resistance1`, `resistance2`, `resistance3`, `resistance4`, `resistance5`, `resistance6`, `spell1`, `spell2`, `spell3`, `spell4`, `spell5`, `spell6`, `spell7`, `spell8`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `InhabitType`, `Health_mod`, `Mana_mod`, `Armor_mod`, `RacialLeader`, `questItem1`, `questItem2`, `questItem3`, `questItem4`, `questItem5`, `questItem6`, `movementId`, `RegenHealth`, `equipment_id`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `WDBVerified`)
VALUES
('52389','52428','0','0','0','0','37867','0','0','0','Eye of Occu\'thar','','','0','87','87','0','14','14','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1.2','1','1','0','0','0','0','0','0','0','94','1','0','0','0','','15595'),
('52428','0','0','0','0','0','37867','0','0','0','Eye of Occu\'thar (1)','','','0','87','87','0','14','14','0','1','1.14286','1','0','0','0','0','0','1','0','0','1','0','0','0','0','0','0','0','0','0','0','10','1024','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','','0','3','1.2','1','1','0','0','0','0','0','0','0','94','1','0','0','0','','15595');

*/


void AddSC_boss_occuthar()
{
    new boss_occuthar();
    new mob_occuthar_eyestalk();
}

