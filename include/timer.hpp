#ifndef __TIMER__HPP
#define __TIMER__HPP
#include "dlist.hpp"
#include "stdint.h"

struct timeSlot
{
    uint64_t idleTime = 0;
    DList timeNode;
};
class timer
{
    DList headNode;
    int k_idle_timeout_ms = 10000;

public:
    void initTimer()
    {
        dlist_init(&headNode);
    }
    static uint64_t get_monotonic_usec();
    bool emptyTimeSLot()
    {
        return dlist_empty(&headNode);
    }
    int64_t getRemainingInterval();
    timeSlot *process_timers();
    void insertTimeslot(timeSlot &instance);
    void timeslotUpdate(timeSlot &instance);
    void deleteTimeslot(timeSlot &instance)
    {
        dlist_detach(&instance.timeNode);
    }
};

#endif