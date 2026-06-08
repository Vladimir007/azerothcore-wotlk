#include "EventMap.h"
#include "Random.h"

void EventMap::Reset()
{
    _eventMap.clear();
    _time = TimePoint::min();
    _phaseMask = 0;
}

void EventMap::SetPhase(const PhaseIndex phase)
{
    if (!phase)
        _phaseMask = 0;
    else if (phase <= sizeof(PhaseMask) * 8)
        _phaseMask = static_cast<PhaseMask>(1u << (phase - 1u));
}

void EventMap::AddPhase(const PhaseIndex phase)
{
    if (phase && phase <= sizeof(PhaseMask) * 8)
        _phaseMask |= static_cast<PhaseMask>(1u << (phase - 1u));
}

void EventMap::RemovePhase(const PhaseIndex phase)
{
    if (phase && phase <= sizeof(PhaseMask) * 8)
        _phaseMask &= static_cast<PhaseMask>(~(1u << (phase - 1u)));
}

void EventMap::ScheduleEvent(const EventId eventId, const Milliseconds time, const GroupIndex group /*= 0u*/, const PhaseIndex phase /*= 0u*/)
{
    if (group > sizeof(GroupMask) * 8)
        return;

    if (phase > sizeof(PhaseMask) * 8)
        return;

    _eventMap.emplace(_time + time, Event(eventId, group, phase));
}

void EventMap::ScheduleEvent(const EventId eventId, const Milliseconds minTime, const Milliseconds maxTime, const GroupIndex group /*= 0u*/, const PhaseIndex phase /*= 0u*/)
{
    ScheduleEvent(eventId, randtime(minTime, maxTime), group, phase);
}

void EventMap::RescheduleEvent(const EventId eventId, const Milliseconds minTime, const Milliseconds maxTime, const GroupIndex group /*= 0u*/, const PhaseIndex phase /*= 0u*/)
{
    CancelEvent(eventId);
    ScheduleEvent(eventId, randtime(minTime, maxTime), group, phase);
}

void EventMap::RescheduleEvent(const EventId eventId, const Milliseconds time, const GroupIndex group /*= 0u*/, const PhaseIndex phase /*= 0u*/)
{
    CancelEvent(eventId);
    ScheduleEvent(eventId, time, group, phase);
}

void EventMap::Repeat(const Milliseconds time)
{
    _eventMap.emplace(_time + time, _lastEvent);
}

void EventMap::Repeat(const Milliseconds minTime, const Milliseconds maxTime)
{
    Repeat(randtime(minTime, maxTime));
}

EventMap::EventId EventMap::ExecuteEvent()
{
    while (!Empty())
    {
        auto const& itr = _eventMap.begin();

        if (itr->first > _time)
            return 0;

        if (_phaseMask && itr->second._phaseMask && !(itr->second._phaseMask & _phaseMask))
            _eventMap.erase(itr);
        else
        {
            const auto eventId = itr->second._id;
            _lastEvent = itr->second;
            _eventMap.erase(itr);
            return eventId;
        }
    }

    return 0;
}

void EventMap::DelayEvents(const Milliseconds delay)
{
    if (Empty())
        return;

    EventStore delayed = std::move(_eventMap);
    for (auto itr = delayed.begin(); itr != delayed.end();)
    {
        auto node = delayed.extract(itr++);
        node.key() = node.key() + delay;
        _eventMap.insert(_eventMap.end(), std::move(node));
    }
}

void EventMap::DelayEvents(const Milliseconds delay, const GroupIndex group)
{
    if (group > sizeof(GroupMask) * 8 || Empty())
        return;

    EventStore delayed;

    for (auto itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (!group || (itr->second._groupMask & static_cast<GroupMask>(1u << (group - 1u))))
        {
            delayed.emplace(itr->first + delay, itr->second);
            itr = _eventMap.erase(itr);
            continue;
        }

        ++itr;
    }

    _eventMap.insert(delayed.begin(), delayed.end());
}

void EventMap::DelayEventsToMax(const Milliseconds delay, const GroupIndex group)
{
    for (auto itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (itr->first < _time + delay && (!group || (itr->second._groupMask & static_cast<GroupMask>(1u << (group - 1u)))))
        {
            ScheduleEvent(itr->second._id, delay, group);
            _eventMap.erase(itr);
            itr = _eventMap.begin();
            continue;
        }

        ++itr;
    }
}

void EventMap::CancelEvent(const EventId eventId)
{
    if (Empty())
        return;

    for (auto itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (eventId == itr->second._id)
        {
            itr = _eventMap.erase(itr);
            continue;
        }

        ++itr;
    }
}

void EventMap::CancelEventGroup(const GroupIndex group)
{
    if (!group || group > sizeof(GroupMask) * 8 || Empty())
        return;

    for (auto itr = _eventMap.begin(); itr != _eventMap.end();)
    {
        if (itr->second._groupMask & static_cast<GroupMask>(1u << (group - 1u)))
        {
            _eventMap.erase(itr);
            itr = _eventMap.begin();
            continue;
        }

        ++itr;
    }
}

bool EventMap::IsInPhase(const PhaseIndex phase) const
{
    return phase <= sizeof(PhaseIndex) * 8 && (!phase || _phaseMask & static_cast<PhaseMask>(1u << (phase - 1u)));
}

Milliseconds EventMap::GetTimeUntilEvent(const EventId eventId) const
{
    for (auto const& [time, event] : _eventMap)
        if (eventId == event._id)
            return std::chrono::duration_cast<Milliseconds>(time - _time);

    return Milliseconds::max();
}

bool EventMap::HasTimeUntilEvent(const EventId eventId) const
{
    return GetTimeUntilEvent(eventId) != Milliseconds::max();
}
