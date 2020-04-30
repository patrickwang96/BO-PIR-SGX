
#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"
#include "time.h"


void ocall_get_time(uint64_t *second, uint64_t *nanosecond)
{
    struct timespec wall_clock;
    clock_gettime(CLOCK_REALTIME, &wall_clock);
    *second = wall_clock.tv_sec;
    *nanosecond = wall_clock.tv_nsec;
}
