// achievement Leeeeeeeroy! by Oj
// blackrock_spire.cpp

#include "ScriptPCH.h"

enum
{
    NPC_ROOKERY_WHELP       = 10161,
    GO_ROOKERY_EGG          = 175124
};

class mob_rookery_whelp : public CreatureScript
{
public:
    mob_rookery_whelp(): CreatureScript("mob_rookery_whelp") { }

    struct mob_rookery_whelpAI : public ScriptedAI
    {
        mob_rookery_whelpAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* pInstance;
        bool m_bEggDestroyed;
        bool m_bWhelpHatched;
        uint32 m_uiHatch_Timer;

        void Reset()
        {
            m_bEggDestroyed = false;
            m_bWhelpHatched = false;
            m_uiHatch_Timer = 2400;
        }

        void JustDied(Unit* pKiller)
        {
            if (pInstance)
                pInstance->SetData(NPC_ROOKERY_WHELP, SPECIAL);
        }

        void EnterCombat(Unit* /*who*/) {}

        void AttackStart(Unit* who)
        {
            if (!m_bWhelpHatched)
                return;

            UnitAI::AttackStart(who);
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!m_bEggDestroyed)
            {
                if (!me->getVictim() && who->isTargetableForAttack() && me->IsHostileTo(who))
                {
                    if (!me->canFly() && me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                        return;

                    if (me->IsWithinDistInMap(who, 5) && me->IsWithinLOSInMap(who))
                    {
                        m_bEggDestroyed = true;
                        if(GameObject* pEgg = GetClosestGameObjectWithEntry(me, GO_ROOKERY_EGG, 4.0f))
                            pEgg->Delete();

                        if(me->GetVisibility() == VISIBILITY_ON)// already hatched before wipe. not to wait again
                            m_bWhelpHatched = true;
                    }
                }
            } else if (m_bWhelpHatched)
            {
                if (!me->getVictim() && who->isTargetableForAttack() && me->IsHostileTo(who))
                {
                    if (!me->canFly() && me->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                        return;

                    float attackRadius = me->GetAttackDistance(who);
                    if (me->IsWithinDistInMap(who, attackRadius) && me->IsWithinLOSInMap(who))
                        AttackStart(who);
                }
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (m_bEggDestroyed && !m_bWhelpHatched)
            {
                if (m_uiHatch_Timer < diff)
                {
                    m_bWhelpHatched = true;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetVisibility(VISIBILITY_ON);
                } else m_uiHatch_Timer -= diff;
            }

            if (!UpdateVictim()) //valid target check
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_rookery_whelpAI(creature);
    }
};
void AddSC_blackrock_spire()
{
    new mob_rookery_whelp();
}
