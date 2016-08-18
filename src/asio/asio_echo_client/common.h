#pragma once

#include <stdint.h>
#include <atomic>

#define MIN_PACKET_SIZE     64
#define MAX_PACKET_SIZE     (64 * 1024)

extern uint32_t g_test_mode;
extern uint32_t g_test_category;

extern std::string g_test_mode_str;
extern std::string g_test_category_str;
extern std::string g_rpc_topic;

extern std::string g_server_ip;
extern std::string g_server_port;

namespace asio_test {

enum test_mode_t {
    mode_unknown,
    mode_pingpong,
    mode_qps,
    mode_throughput,
    mode_latency
};

} // namespace asio_test

