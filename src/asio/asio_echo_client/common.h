#pragma once

#include <atomic>

enum test_mode_t {
    mode_unknown,
    mode_pingpong,
    mode_qps,
    mode_delay,
    mode_throughout
};

extern std::uint32_t g_mode;
extern std::string g_mode_str;
