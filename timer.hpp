#ifndef __TIMER__HPP
#define __TIMER__HPP
#include "dlist.hpp"
#include "stdint.h"
#include "helper.hpp"

struct timeSlot
{
    int idleTime = 0;
    DList timeNode;
};
class timer
{
    DList headNode;
    int k_idle_timeout_ms = 5 * 1000;
public:
    void initTimer()
    {
        dlist_init(&headNode);
    }
    static uint64_t get_monotonic_usec();
    int64_t next_timer_ms();
    timeSlot* process_timers();
    int insertTimeslot(timeSlot &instance);
    int timeslotUpdate(timeSlot &instance);
    void deleteTimeslot(timeSlot &instance)
    {
        dlist_detach(&instance.timeNode);
    }
};
uint64_t timer::get_monotonic_usec()
{
    timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_nsec / 1000;
}
timeSlot* timer::process_timers()
{
    uint64_t now_us = get_monotonic_usec();
    while (!dlist_empty(&headNode))
    {
        timeSlot *nearest = container_of(headNode.next, timeSlot, timeNode);
        uint64_t next_us = nearest->idleTime + k_idle_timeout_ms * 1000;
        if (next_us >= now_us + 1000)
        {
            // not ready, the extra 1000us is for the ms resolution of poll()
            return nullptr;
        }
        return nearest;
    }
}
int64_t timer::next_timer_ms()
{
    if (dlist_empty(&headNode)) {
        return 10000;   // no timer, the value doesn't matter
    }

    uint64_t now_us = get_monotonic_usec();
    timeSlot *nearest = container_of(headNode.next, timeSlot, timeNode);
    uint64_t next_us = nearest->idleTime + k_idle_timeout_ms * 1000;
    if (next_us <= now_us) {
        // missed?
        return 0;
    }

    return (uint32_t)((next_us - now_us) / 1000);
}
int timer:: timeslotUpdate(timeSlot &instance)
{
    dlist_detach(&instance.timeNode);
    dlist_insert_last(&headNode, &instance.timeNode);
}
int timer:: insertTimeslot(timeSlot &instance)
{
    instance.idleTime = get_monotonic_usec();
    dlist_insert_last(&headNode, &instance.timeNode);
}
#endif