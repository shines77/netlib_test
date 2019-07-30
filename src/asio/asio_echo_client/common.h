#pragma once

#include <stdint.h>
#include <string>
#include <atomic>

#define MIN_PACKET_SIZE     64
#define MAX_PACKET_SIZE     (64 * 1024)

extern uint32_t g_test_mode;
extern uint32_t g_test_method;

extern std::string g_test_mode_str;
extern std::string g_test_method_str;
extern std::string g_rpc_topic;

extern std::string g_server_ip;
extern std::string g_server_port;

namespace asio_test {

enum test_mode_t {
    test_mode_unknown,
    test_mode_echo,
    test_mode_http,
    test_mode_last
};

enum test_method_t {
    test_method_unknown,
    test_method_pingpong,
    test_method_qps,
    test_method_throughput,
    test_method_latency,
    test_method_last
};

} // namespace asio_test

