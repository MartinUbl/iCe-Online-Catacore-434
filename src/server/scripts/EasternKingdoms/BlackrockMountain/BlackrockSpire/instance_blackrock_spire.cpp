// achievement Leeeeeeeroy! by Oj
// instance_blackrock_spire.cpp

#include "ScriptPCH.h"

enum
{
    NPC_ROOKERY_WHELP       = 10161,
    GO_ROOKERY_EGG          = 175124,
    ACHIEVEMENT_LEEROY      = 2188,
    MAP_BRS					= 229
};

class instance_blackrock_spire : public InstanceMapScript
{
public:
    instance_blackrock_spire() : InstanceMapScript("instance_blackrock_spire", MAP_BRS) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_blackrock_spire_InstanceMapScript(pMap);
    }

    struct instance_blackrock_spire_InstanceMapScript : public InstanceScript
    {
        instance_blackrock_spire_InstanceMapScript(Map* pMap) : InstanceScript(pMap)
		{
			Initialize();
		};


		std::list<uint32> m_uiRookeryWhelpDeathTimeStamp;
		int m_iRookeryWhelpDeathCounter;
		int m_iLeeroyAchievementCounter;

		void Initialize() { LeeroyReset(); }

		void OnGameObjectCreate(GameObject *pGo, bool add)
		{
            if (!add)
                return;

			if (pGo->GetEntry() == GO_ROOKERY_EGG)
			{
				float ang = float(urand(1, 628)) / 100;
				Creature* pWhelp = pGo->SummonCreature(NPC_ROOKERY_WHELP, pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), ang, TEMPSUMMON_DEAD_DESPAWN, 0);
				if (pWhelp)
				{
					pWhelp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					pWhelp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
					pWhelp->SetVisibility(VISIBILITY_OFF);
					pWhelp = NULL;
				}
			}
        }

        void LeeroyReset()
        {
			m_uiRookeryWhelpDeathTimeStamp.clear();
			m_iRookeryWhelpDeathCounter = 0;
			m_iLeeroyAchievementCounter = 0;
        }

        void SetData(uint32 uiType, uint32 uiData)
        {
            switch (uiType)
            {
                default: break;
                case NPC_ROOKERY_WHELP:
                {
                    RecordDeathTimeStamp();
                    if (TestLeeroyAchievement())
						DoCompleteAchievement(ACHIEVEMENT_LEEROY);
                    break;
                }
            }
        }

        void RecordDeathTimeStamp()
        {
			m_uiRookeryWhelpDeathTimeStamp.push_back(getMSTime());
			m_iRookeryWhelpDeathCounter++;
        }

        bool TestLeeroyAchievement()
        {
            if (m_iRookeryWhelpDeathCounter >= 49)
            {
				uint32 m_uiTime = getMSTime();
				m_iLeeroyAchievementCounter = 0;
				for(std::list<uint32>::const_iterator itr = m_uiRookeryWhelpDeathTimeStamp.begin(); itr != m_uiRookeryWhelpDeathTimeStamp.end(); ++itr)
				{
					if (*itr >= (m_uiTime - 15000))
						m_iLeeroyAchievementCounter++;
				}
				if (m_iLeeroyAchievementCounter == 50)
					return true;
            }
            return false;
        }
	};
};
void AddSC_instance_blackrock_spire()
{
    new instance_blackrock_spire();
}
