#pragma once

#include <atomic>

namespace asio_test {

enum test_mode_t {
    mode_unknown,
    mode_pingpong,
    mode_qps,
    mode_latency,
    mode_throughout
};

} // namespace asio_test

extern uint32_t g_mode;
extern std::string g_mode_str;
