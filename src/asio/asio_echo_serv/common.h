#pragma once

#include <atomic>
#include "common/padding_atomic.hpp"

extern std::atomic<uint64_t> g_query_count_;
extern std::atomic<uint32_t> g_client_count_;

namespace asio_test {
    extern padding_atomic<uint64_t> g_query_count;
    extern padding_atomic<uint32_t> g_client_count;

    extern padding_atomic<uint64_t> g_recieved_bytes;
    extern padding_atomic<uint64_t> g_sent_bytes;
}
