/*
 * Copyright (C) 2006-2016, iCe Online <http://ice-wow.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef GCORE_QUERYSCHEDULER_H
#define GCORE_QUERYSCHEDULER_H

#include <ace/Singleton.h>
#include <queue>

enum QuerySchedulerFlags
{
    QSF_RUN_AFTER_CLEANUP       = 0x0001,       // run only after morning maintenance, partially time-scheduled (may specify day of week)
    QSF_RUN_AFTER_STARTUP       = 0x0002,       // run only after emulator startup, no time-based scheduling
    QSF_RUN_ONCE                = 0x0004,       // run only once (checks presence of dynamic flag QSF_DYNAMIC_RAN_ONCE - if present, no execution will occur)
    QSF_TIME_EXACT              = 0x0008,       // will be scheduled using nextRun instead of current time; this will avoid cummulative delaying
};

enum QuerySchedulerDynamicFlags
{
    QSF_DYNAMIC_CLEANUP_DONE    = 0x0001,       // the morning maintenance just happened, execute this query!
    QSF_DYNAMIC_RAN_ONCE        = 0x0002,       // the query was executed before, if QSF_RUN_ONCE specified, never ever run that query
};

struct QuerySchedulerEntry
{
    // entry ID
    uint32 id;
    // timestamp of next run time
    time_t nextRun;
    // time between runs; if 0, no rescheduling will happen and QSF_RUN_ONCE will be set
    time_t period;
    // mask of days, at which the script should run (1 << dow, localtime day of week numbering)
    uint32 dayOfWeekMask;
    // script static flags; see QuerySchedulerFlags enum
    uint32 flags;
    // script dynamic flags; see QuerySchedulerDynamicFlags enum
    uint32 dynamicFlags;
    // queries to be executed
    std::string queries;

    bool HasFlag(uint32 flag)
    {
        return (flags & flag) != 0;
    }

    bool HasDynamicFlag(uint32 dynamicFlag)
    {
        return (dynamicFlags & dynamicFlag) != 0;
    }
};

struct QuerySchedulerPriorityQueueComparator
{
    bool operator()(QuerySchedulerEntry const* first, QuerySchedulerEntry const* second)
    {
        return first->nextRun > second->nextRun;
    }
};

class QueryScheduler
{
    public:
        QueryScheduler();

        void LoadFromDB();
        void CheckStartupSchedules();
        void Update();

    protected:
        void ExecuteQuery(QuerySchedulerEntry* entry);
        bool CheckDayOfWeekMask(QuerySchedulerEntry* entry);
        bool CheckRunOnceFlags(QuerySchedulerEntry* entry);
        bool RescheduleEntry(QuerySchedulerEntry* entry);

        void SaveEntryToDB(QuerySchedulerEntry* entry);

    private:
        // priority queue of time-scheduled entries; the most imminent scheduled query is in front (top)
        std::priority_queue<QuerySchedulerEntry*, std::vector<QuerySchedulerEntry*>, QuerySchedulerPriorityQueueComparator> m_timeScheduledQueries;
        // miscellanous non-time-scheduled queries
        std::list<QuerySchedulerEntry*> m_miscQueries;
};

#define sQueryScheduler ACE_Singleton<QueryScheduler, ACE_Null_Mutex>::instance()

#endif
