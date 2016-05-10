
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>

#include "common.h"
#include "async_asio_echo_serv.hpp"

std::atomic<uint64_t> g_query_count_(0);
std::atomic<uint32_t> g_client_count_(0);

asio_test::padding_atomic<uint64_t> asio_test::g_query_count(0);
asio_test::padding_atomic<uint32_t> asio_test::g_client_count(0);

using namespace asio_test;

std::string get_app_name(char * app_exe)
{
    std::string app_name;
    std::size_t len = std::strlen(app_exe);
    char * end_ptr = app_exe;
    char * begin_ptr = app_exe + len;
    char * cur_ptr = begin_ptr;
    while (cur_ptr >= end_ptr) {
        if (*cur_ptr == '/' || *cur_ptr == '\\') {
            if (cur_ptr != begin_ptr) {
                break;
            }
        }
        cur_ptr--;
    }
    cur_ptr++;
    app_name = cur_ptr;
    return app_name;
}

int parse_number_u32(std::string::const_iterator & iterBegin,
    const std::string::const_iterator & iterEnd, unsigned int & num)
{
    int n = 0, digits = 0;
    std::string::const_iterator & iter = iterBegin;
    for (; iter != iterEnd; ++iter) {
        char ch = *iter;
        if (ch >= '0' && ch <= '9') {
            n = n * 10 + ch - '0';
            digits++;
        }
        else {
            break;
        }
    }
    if (digits > 0) {
        if (digits <= 10)
            num = n;
        else
            digits = 0;
    }
    return digits;
}

int parse_number_u32(const std::string & str, unsigned int & num)
{
    std::string::const_iterator iter = str.begin();
    return parse_number_u32(iter, str.end(), num);
}

bool is_valid_ip_v4(const std::string & ip)
{
    if (ip.empty() || ip.length() > 15)
        return false;

    unsigned int num;
    int digits = 0;
    int dots = 0;
    int num_cnt = 0;
    std::string::const_iterator iter;
    for (iter = ip.begin(); iter != ip.end(); ++iter) {
        digits = parse_number_u32(iter, ip.end(), num);
        if ((digits > 0) && (/*num >= 0 && */num < 256)) {
            num_cnt++;
            if (iter == ip.end())
                break;
            char ch = *iter;
            if (ch == '.')
                dots++;
            else
                break;
        }
        else
            return false;
    }
    return (dots == 3 && num_cnt == 4);
}

bool is_number_u32(const std::string & str)
{
    if (str.empty() || str.length() > 5)
        return false;

    std::string::const_iterator iter;
    for (iter = str.begin(); iter != str.end(); ++iter) {
        char ch = *iter;
        if (ch < '0' || ch > '9')
            return false;
    }
    return true;
}

bool is_socket_port(const std::string & port)
{
    unsigned int port_num = 0;
    int digits = parse_number_u32(port, port_num);
    return ((digits > 0) && (port_num > 0 && port_num < 65536));
}

void print_usage(const std::string & app_name)
{
    std::cerr << "Usage: " << app_name.c_str() << " <ip> <port> [<packet_size> = 64] [<thread_cnt> = hw_cpu_cores]" << std::endl << std::endl
              << "       For example: " << app_name.c_str() << " 192.168.2.154 8090 64 8" << std::endl;
}

int main(int argc, char * argv[])
{
    std::string app_name;
    app_name = get_app_name(argv[0]);
    if (argc <= 2) {
        print_usage(app_name);
        return 1;
    }

    std::string ip, port;
    uint32_t packet_size = 0, thread_cnt = 0;

    ip = argv[1];
    if (!is_valid_ip_v4(ip)) {
        //ip = "127.0.0.1";
        std::cerr << "Error: ip address \"" << argv[1] << "\" format is wrong." << std::endl;
        return 1;
    }

    port = argv[2];
    if (!is_socket_port(port)) {
        //port = "8090";
        std::cerr << "Error: port [" << argv[1] << "] number must be range in (0, 65535]." << std::endl;
        return 1;
    }

    if (argc > 3)
        packet_size = atoi(argv[3]);
    if (packet_size <= 0)
        packet_size = 64;
    if (packet_size > MAX_PACKET_SIZE) {
        std::cerr << "Warnning: packet_size = " << packet_size << " is more than "
                  << MAX_PACKET_SIZE << " bytes [MAX_PACKET_SIZE]." << std::endl;
        packet_size = MAX_PACKET_SIZE;
    }

    if (argc > 4)
        thread_cnt = atoi(argv[4]);
    if (thread_cnt <= 0)
        thread_cnt = std::thread::hardware_concurrency();

    std::cout << argv[0] << " begin ..." << std::endl;
    std::cout << std::endl;
    std::cout << "listen " << ip.c_str() << ":" << port.c_str() << std::endl;
    std::cout << "packet_size: " << packet_size << ", thread_cnt: " << thread_cnt << std::endl;
    std::cout << std::endl;

    try
    {
        //async_asio_echo_serv server("192.168.2.191", "8090", 64, std::thread::hardware_concurrency());
        //async_asio_echo_serv server(8090, std::thread::hardware_concurrency());
        async_asio_echo_serv server(ip, port, packet_size, thread_cnt);
        server.run();

        std::cout << "Server has bind and listening ..." << std::endl;
        //std::cout << "press [enter] key to continue ...";
        //getchar();
        std::cout << std::endl;

        std::uint64_t last_query_count = 0;
        while (true) {
            auto curr_succeed_count = (std::uint64_t)g_query_count;
            auto client_count = (std::uint32_t)g_client_count;
            std::cout << "[" << client_count << "] conn - " << thread_cnt << " thread : " << packet_size << "B : qps = "
                      << (curr_succeed_count - last_query_count) << std::endl;
            last_query_count = curr_succeed_count;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        server.join();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
