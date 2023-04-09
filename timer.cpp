#include "timer.hpp"
#include "time.h"
uint64_t timer:: get_monotonic_usec() 
{
    timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return uint64_t(tv.tv_sec) * 1000000 + tv.tv_nsec / 1000;
}