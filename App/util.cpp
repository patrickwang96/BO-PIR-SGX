
#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"
#include "time.h"


void ocall_get_time(long *second, long *nanosecond)
{
    struct timespec wall_clock;
    clock_gettime(CLOCK_REALTIME, &wall_clock);
    *second = wall_clock.tv_sec;
    *nanosecond = wall_clock.tv_nsec;
}
