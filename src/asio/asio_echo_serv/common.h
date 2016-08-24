#pragma once

#include <stdint.h>
#include <atomic>
#include "common/padding_atomic.hpp"

extern uint32_t g_test_mode;
extern uint32_t g_test_method;

extern std::string g_test_mode_str;
extern std::string g_test_method_str;
extern std::string g_rpc_topic;

extern std::string g_server_ip;
extern std::string g_server_port;

namespace asio_test {

enum session_mode_t {
    mode_need_echo,
    mode_dont_need_echo
};

enum test_mode_t {
    test_mode_unknown,
    test_mode_echo,
    test_mode_rpc_call,
    test_mode_sub_pub,
    test_mode_default = -1
};

enum test_method_t {
    test_method_unknown,
    test_method_pingpong,
    test_method_qps,
    test_method_throughput,
    test_method_latency,
    test_method_async_qps,
    test_method_default = -1
};

extern padding_atomic<uint64_t> g_query_count;
extern padding_atomic<uint32_t> g_client_count;

extern padding_atomic<uint64_t> g_recieved_bytes;
extern padding_atomic<uint64_t> g_sent_bytes;

}
