#ifndef EVENT_PROCESSOR_H
#define EVENT_PROCESSOR_H

#include "Define.h"
#include "Duration.h"
#include "Random.h"
#include <map>

class EventProcessor;

// Note. All times are in milliseconds here.
class BasicEvent
{
    friend class EventProcessor;

    enum class AbortState : uint8
    {
        STATE_RUNNING,
        STATE_ABORT_SCHEDULED,
        STATE_ABORTED
    };

public:
    BasicEvent() = default;
    virtual ~BasicEvent() = default; // Override destructor to perform some actions on event removal

    // This method executes when the event is triggered.
    // Return false if event does not want to be deleted.
    // e_time is execution time, p_time is update interval.
    [[nodiscard]] virtual bool Execute(uint64 /*e_time*/, uint32 /*p_time*/) { return true; }

    // This event can be safely deleted
    [[nodiscard]] virtual bool IsDeletable() const { return true; }

    // This method executes when the event is aborted
    virtual void Abort(uint64 /*e_time*/) {}

    // Aborts the event at the next update tick
    void ScheduleAbort();

private:
    void SetAborted();
    [[nodiscard]] bool IsRunning() const { return m_abortState == AbortState::STATE_RUNNING; }
    [[nodiscard]] bool IsAbortScheduled() const { return m_abortState == AbortState::STATE_ABORT_SCHEDULED; }
    [[nodiscard]] bool IsAborted() const { return m_abortState == AbortState::STATE_ABORTED; }

    // Set by externals when the event is aborted, aborted events don't execute
    AbortState m_abortState{AbortState::STATE_RUNNING};

    // These can be used for time offset control
    uint64 m_addTime{0};  // Time when the event was added to queue, filled by event handler
    uint64 m_execTime{0};  // Planned time of next execution, filled by event handler
    uint8 m_eventGroup{0};
};

template<typename T>
class LambdaBasicEvent : public BasicEvent
{
public:
    explicit LambdaBasicEvent(T&& callback) : BasicEvent(), _callback(std::move(callback)) { }

    bool Execute(uint64, uint32) override
    {
        _callback();
        return true;
    }

private:
    T _callback;
};

template<typename T>
using is_lambda_event = std::enable_if_t<!std::is_base_of_v<BasicEvent, std::remove_pointer_t<std::remove_cvref_t<T>>>>;

typedef std::multimap<uint64, BasicEvent*> EventList;

class EventProcessor
{
    public:
        EventProcessor()  = default;
        ~EventProcessor();

        void Update(uint32 p_time);
        void KillAllEvents(bool force);

        void AddEvent(BasicEvent* Event, uint64 e_time, bool set_addTime = true, uint8 eventGroup = 0);

        template<typename T>
        is_lambda_event<T> AddEvent(T&& event, const Milliseconds e_time, bool set_addTime = true, uint8 eventGroup = 0)
        {
            AddEvent(new LambdaBasicEvent<T>(std::move(event)), e_time.count(), set_addTime, eventGroup);
        }

        void AddEventAtOffset(BasicEvent* event, const Milliseconds offset, const uint8 eventGroup = 0)
        {
            AddEvent(event, CalculateTime(offset.count()), true, eventGroup);
        }

        template<typename T>
        is_lambda_event<T> AddEventAtOffset(T&& event, Milliseconds offset, uint8 eventGroup = 0)
        {
            AddEventAtOffset(new LambdaBasicEvent<T>(std::move(event)), offset, eventGroup);
        }

        void AddEventAtOffset(BasicEvent* event, const Milliseconds offset, const Milliseconds offset2, const uint8 eventGroup = 0)
        {
            AddEvent(event, CalculateTime(randtime(offset, offset2).count()), true, eventGroup);
        }

        template<typename T>
        is_lambda_event<T> AddEventAtOffset(T&& event, Milliseconds offset, Milliseconds offset2, uint8 eventGroup = 0)
        {
            AddEventAtOffset(new LambdaBasicEvent<T>(std::move(event)), offset, offset2, eventGroup);
        }

        void ModifyEventTime(BasicEvent* event, Milliseconds newTime);
        [[nodiscard]] uint64 CalculateTime(uint64 t_offset) const;

        // Calculates next queue tick time
        [[nodiscard]] uint64 CalculateQueueTime(uint64 delay) const;

        void CancelEventGroup(uint8 group);
        bool HasEvents() const { return !m_events.empty(); }

    protected:
        uint64 m_time{0};
        EventList m_events;
        bool m_aborting;
};

#endif
