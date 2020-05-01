
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

extern boost::asio::io_service service;
extern boost::asio::ip::tcp::endpoint ep(ip::address::from_string(ADDRESS), PORT);
extern boost::asio::ip::tcp::socket sock(service);

void ocall_send(char* data, uint64_t size) {
    sock.write_some(boost::asio::buffer(data, size));
}

void ocall_recv(char* data, uint64_t size) {
    sock.read_some(boost::asio::buffer(data, size));
}