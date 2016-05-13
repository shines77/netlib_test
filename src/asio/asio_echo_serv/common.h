#pragma once

#include <atomic>
#include "common/padding_atomic.hpp"

extern uint32_t     g_mode;
extern std::string  g_mode_str;

namespace asio_test {

enum session_mode_t {
    mode_need_respond,
    mode_no_respond
};

extern padding_atomic<uint64_t> g_query_count;
extern padding_atomic<uint32_t> g_client_count;

extern padding_atomic<uint64_t> g_recieved_bytes;
extern padding_atomic<uint64_t> g_sent_bytes;

}
