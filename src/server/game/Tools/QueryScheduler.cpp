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

#include "gamePCH.h"
#include "QueryScheduler.h"

QueryScheduler::QueryScheduler()
{
    //
}

void QueryScheduler::LoadFromDB()
{
    // cleanup in case of reload
    m_miscQueries.clear();
    while (!m_timeScheduledQueries.empty())
        m_timeScheduledQueries.pop();

    //                                             0,        1,      2,                3,     4,             5,     6
    QueryResult res = LoginDatabase.Query("SELECT id, next_run, period, day_of_week_mask, flags, dynamic_flags, query FROM query_scheduler WHERE disabled = 0");

    QuerySchedulerEntry* qsentry;

    if (res)
    {
        do
        {
            Field* f = res->Fetch();
            if (!f)
                break;

            qsentry = new QuerySchedulerEntry;

            qsentry->id = f[0].GetUInt32();
            qsentry->nextRun = (time_t)f[1].GetUInt64();
            qsentry->period = (time_t)f[2].GetUInt32();
            qsentry->dayOfWeekMask = f[3].GetUInt32();
            qsentry->flags = f[4].GetUInt32();
            qsentry->dynamicFlags = f[5].GetUInt32();
            qsentry->queries = f[6].GetCString();

            // do not count expired queries
            if (qsentry->HasFlag(QSF_RUN_ONCE) && qsentry->HasDynamicFlag(QSF_DYNAMIC_RAN_ONCE))
            {
                sLog->outError("Inactive scheduled query (ID: %u) present in database; remove it or set disabled flag");
                delete qsentry;
                continue;
            }

            if (qsentry->HasFlag(QSF_RUN_AFTER_CLEANUP) || qsentry->HasFlag(QSF_RUN_AFTER_STARTUP))
                m_miscQueries.push_back(qsentry);
            else
                m_timeScheduledQueries.push(qsentry);
        }
        while (res->NextRow());
    }
}

void QueryScheduler::SaveEntryToDB(QuerySchedulerEntry* entry)
{
    LoginDatabase.PExecute("UPDATE query_scheduler SET next_run = " UI64FMTD ", period = %u, day_of_week_mask = %u, flags = %u, dynamic_flags = %u WHERE id = %u",
        (uint64)entry->nextRun, (uint32)entry->period, entry->dayOfWeekMask, entry->flags, entry->dynamicFlags, entry->id);
}

void QueryScheduler::CheckStartupSchedules()
{
    // go through misc queries, since startup queries are not time-scheduled
    for (QuerySchedulerEntry* qsentry : m_miscQueries)
    {
        // if the "startup" flags is present, OR the "after cleanup" flag is present and we are starting after cleanup
        if (qsentry->HasFlag(QSF_RUN_AFTER_STARTUP) || (qsentry->HasFlag(QSF_RUN_AFTER_CLEANUP) && qsentry->HasDynamicFlag(QSF_DYNAMIC_CLEANUP_DONE)))
        {
            // check conditions to run this query (is not timed or the timer has passed, day of week condition, run once condition)
            if ((qsentry->nextRun == 0 || qsentry->nextRun < time(nullptr)) && CheckDayOfWeekMask(qsentry) && CheckRunOnceFlags(qsentry))
            {
                ExecuteQuery(qsentry);

                // reschedule if needed
                if (qsentry->nextRun != 0)
                    RescheduleEntry(qsentry);

                SaveEntryToDB(qsentry);
            }
        }
    }

    // remove "after cleanup" dynamic flag
    LoginDatabase.PExecute("UPDATE query_scheduler SET dynamic_flags = dynamic_flags & ~%u", QSF_DYNAMIC_CLEANUP_DONE);
}

void QueryScheduler::Update()
{
    if (m_timeScheduledQueries.empty())
        return;

    bool reschedule;
    QuerySchedulerEntry* qsentry;

    while (!m_timeScheduledQueries.empty() && m_timeScheduledQueries.top()->nextRun <= time(nullptr))
    {
        qsentry = m_timeScheduledQueries.top();
        m_timeScheduledQueries.pop();

        if (CheckDayOfWeekMask(qsentry) && CheckRunOnceFlags(qsentry))
            ExecuteQuery(qsentry);

        reschedule = RescheduleEntry(qsentry);

        SaveEntryToDB(qsentry);

        if (reschedule)
            m_timeScheduledQueries.push(qsentry);
    }
}

void QueryScheduler::ExecuteQuery(QuerySchedulerEntry* entry)
{
    // abuse character log for this kind of stuff
    sLog->outChar("Executing scheduled query (ID: %u) (sched. time: " UI64FMTD ")", entry->id, (uint64)entry->nextRun);

    // execute on world database, since not-fully qualified names would not cause such harm
    // make the query async, so it's enqueued to be executed and does not block this thread
    WorldDatabase.AsyncQuery(entry->queries.c_str());

    entry->dynamicFlags |= QSF_DYNAMIC_RAN_ONCE;
}

bool QueryScheduler::RescheduleEntry(QuerySchedulerEntry* entry)
{
    // for time-based scheduling, period needs to be set
    if (entry->period > 0)
    {
        // exact scheduler flag
        if (entry->HasFlag(QSF_TIME_EXACT))
        {
            // we need to be sure the time is in the future
            while (entry->nextRun <= time(nullptr))
                entry->nextRun = entry->nextRun + entry->period;
        }
        else
            entry->nextRun = time(nullptr) + entry->period;

        return true;
    }
    else // no period means "do not repeat"
        entry->flags |= QSF_RUN_ONCE;

    return false;
}

bool QueryScheduler::CheckDayOfWeekMask(QuerySchedulerEntry* entry)
{
    if (entry->dayOfWeekMask == 0)
        return true;

    time_t secs = time(NULL);
    tm* timeinfo = localtime(&secs);

    /* 0 - sunday
     * 1 - monday
     * 2 - tuesday
     * 3 - wednesday
     * 4 - thursday
     * 5 - friday
     * 6 - saturday */

    if (((1 << timeinfo->tm_wday) & entry->dayOfWeekMask) != 0)
        return true;

    return false;
}

bool QueryScheduler::CheckRunOnceFlags(QuerySchedulerEntry* entry)
{
    return !(entry->HasFlag(QSF_RUN_ONCE) && entry->HasDynamicFlag(QSF_DYNAMIC_RAN_ONCE));
}
