#include "EventProcessor.h"
#include "Errors.h"

void BasicEvent::ScheduleAbort()
{
    ASSERT(IsRunning() && "Tried to scheduled the abortion of an event twice!");
    m_abortState = AbortState::STATE_ABORT_SCHEDULED;
}

void BasicEvent::SetAborted()
{
    ASSERT(!IsAborted() && "Tried to abort an already aborted event!");
    m_abortState = AbortState::STATE_ABORTED;
}

EventProcessor::~EventProcessor()
{
    KillAllEvents(true);
}

void EventProcessor::Update(const uint32 p_time)
{
    // Update time
    m_time += p_time;

    // Main event loop
    EventList::iterator i;
    while ((i = m_events.begin()) != m_events.end() && i->first <= m_time)
    {
        // get and remove event from queue
        BasicEvent* event = i->second;
        m_events.erase(i);

        if (event->IsRunning())
        {
            if (event->Execute(m_time, p_time))
                delete event;  // Completely destroy event if it is not re-added
            continue;
        }

        if (event->IsAbortScheduled())
        {
            event->Abort(m_time);
            event->SetAborted();  // Mark the event as aborted
        }

        if (event->IsDeletable())
        {
            delete event;
            continue;
        }

        // Reschedule non-deletable events to be checked at the next update tick
        AddEvent(event, CalculateTime(1), false);
    }
}

void EventProcessor::KillAllEvents(const bool force)
{
    // First, abort all existing events
    for (auto itr = m_events.begin(); itr != m_events.end();)
    {
        // Abort events which weren't aborted already
        if (!itr->second->IsAborted())
        {
            itr->second->SetAborted();
            itr->second->Abort(m_time);
        }

        // Skip non-deletable events when we are not forcing the event cancellation.
        if (!force && !itr->second->IsDeletable())
        {
            ++itr;
            continue;
        }

        delete itr->second;

        if (force)
            ++itr; // Clear the whole container when forcing
        else
            itr = m_events.erase(itr);
    }

    if (force)
        m_events.clear();
}

void EventProcessor::CancelEventGroup(const uint8 group)
{
    for (auto itr = m_events.begin(); itr != m_events.end();)
    {
        if (itr->second->m_eventGroup != group)
        {
            ++itr;
            continue;
        }

        // Abort events which weren't aborted already
        if (!itr->second->IsAborted())
        {
            itr->second->SetAborted();
            itr->second->Abort(m_time);
        }

        delete itr->second;
        itr = m_events.erase(itr);
    }
}

void EventProcessor::AddEvent(BasicEvent* Event, uint64 e_time, const bool set_addTime /*= true*/, const uint8 eventGroup /*= 0*/)
{
    if (set_addTime)
        Event->m_addTime = m_time;
    Event->m_execTime = e_time;
    Event->m_eventGroup = eventGroup;
    m_events.emplace(e_time, Event);
}

void EventProcessor::ModifyEventTime(BasicEvent* event, const Milliseconds newTime)
{
    for (auto itr = m_events.begin(); itr != m_events.end(); ++itr)
    {
        if (itr->second != event)
            continue;

        event->m_execTime = newTime.count();
        m_events.erase(itr);
        m_events.emplace(newTime.count(), event);
        break;
    }
}

uint64 EventProcessor::CalculateTime(const uint64 t_offset) const
{
    return (m_time + t_offset);
}

uint64 EventProcessor::CalculateQueueTime(const uint64 delay) const
{
    return CalculateTime(delay - (m_time % delay));
}
