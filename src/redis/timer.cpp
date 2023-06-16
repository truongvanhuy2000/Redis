#include "redis/timer.hpp"
#include "redis/helper.hpp"
#include "redis/marco.hpp"

uint64_t timer::get_monotonic_usec()
{
    timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_nsec / 1000;
}
timeSlot *timer::process_timers()
{
    uint64_t now_us = get_monotonic_usec();
    timeSlot *nearest = container_of(headNode.next, timeSlot, timeNode);
    uint64_t next_us = nearest->idleTime + k_idle_timeout_ms * 1000;
    if (next_us >= now_us + 1000)
    {
        // not ready, the extra 1000us is for the ms resolution of poll()
        return nullptr;
    }
    return nearest;
}
// THis func will return the interval between now until the nearest time out point of a connection
int64_t timer::getRemainingInterval()
{
    if (dlist_empty(&headNode))
    {
        return 10000; // no timer, the value doesn't matter
    }

    uint64_t now_us = get_monotonic_usec();
    timeSlot *nearest = container_of(headNode.next, timeSlot, timeNode);
    // std::cout << nearest->idleTime << std::endl;
    // std::cout << now_us << std::endl;
    uint64_t next_us = nearest->idleTime + k_idle_timeout_ms * 1000;
    if (next_us <= now_us)
    {
        // missed?
        return 0;
    }

    return (uint32_t)((next_us - now_us) / 1000);
}
void timer::timeslotUpdate(timeSlot &instance)
{
    instance.idleTime = get_monotonic_usec();
    dlist_detach(&instance.timeNode);
    dlist_insert_last(&headNode, &instance.timeNode);
}
void timer::insertTimeslot(timeSlot &instance)
{
    instance.idleTime = get_monotonic_usec();

    dlist_insert_last(&headNode, &instance.timeNode);
}