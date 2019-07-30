#pragma once

#include <stdint.h>
#include <string>
#include <atomic>
#include "common/aligned_atomic.hpp"

extern uint32_t g_test_mode;
extern uint32_t g_test_method;
extern uint32_t g_nodelay;
extern uint32_t g_need_echo;
extern uint32_t g_packet_size;

extern std::string g_test_mode_str;
extern std::string g_test_method_str;
extern std::string g_test_mode_full_str;
extern std::string g_nodelay_str;
extern std::string g_rpc_topic;

extern std::string g_server_ip;
extern std::string g_server_port;

namespace asio_test {

enum session_mode_t {
    mode_no_echo = 0,
    mode_need_echo = 1
};

enum test_mode_t {
    test_mode_unknown,
    test_mode_echo_server,
    test_mode_no_echo_server,
    test_mode_http_server,
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

extern aligned_atomic<uint64_t> g_query_count;
extern aligned_atomic<uint32_t> g_client_count;

extern aligned_atomic<uint64_t> g_recv_bytes;
extern aligned_atomic<uint64_t> g_send_bytes;

extern const std::string g_response_html;

}
