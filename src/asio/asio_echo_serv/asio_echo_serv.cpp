
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>

#include "async_asio_echo_serv.hpp"

using namespace asio_test;

std::atomic<uint64_t> g_query_count(0);
std::atomic<uint32_t> g_client_count(0);

int parse_number_u32(std::string::const_iterator & iterBegin,
    const std::string::const_iterator & iterEnd, unsigned int & num)
{
    int n = 0, digits = 0;
    std::string::const_iterator iter;
    for (iter = iterBegin; iter != iterEnd; ++iter) {
        char ch = *iter;
        if (ch >= '0' && ch <= '9') {
            n = n * 10 + ch - '0';
            digits++;
        }
        else {
            if (digits > 0) {
                if (digits <= 10)
                    num = n;
                else
                    digits = 0;
            }
            break;
        }
    }
    return digits;
}

int parse_number_u32(const std::string & str, unsigned int & num)
{
    return parse_number_u32(str.begin(), str.end(), num);
}

bool is_valid_ip_v4(const std::string & ip)
{
    if (ip.empty() || ip.length() > 15)
        return false;

    unsigned int num;
    int digits = 0;
    int dots = 0;
    std::string::const_iterator iter;
    for (iter = ip.begin(); iter != ip.end(); ++iter) {
        digits = parse_number_u32(iter, ip.end(), num);
        if ((digits > 0) && (num >= 0 && num < 256)) {
            char ch = *iter;
            if (ch == '.')
                dots++;
            else
                break;
        }
        else
            return false;
    }
    return (dots == 3);
}

bool is_number_u32(const std::string & str)
{
    if (str.empty() || str.length() > 5)
        return false;

    unsigned int num = 0;
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

int main(int argc, char * argv[])
{
    if (argc <= 2) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> [<packet_size> = 64] [<thread_cnt> = hw_cpu_cores]" << std::endl;
        return 1;
    }

    std::string ip, port;
    uint32_t packet_size = 0, thread_cnt = 0;

    ip = argv[1];
    if (!is_valid_ip_v4(ip))
        ip = "127.0.0.1";

    port = argv[2];
    if (!is_socket_port(port))
        port = "8090";

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

    //async_asio_echo_serv server("192.168.2.191", "8090", 64, std::thread::hardware_concurrency());
    //async_asio_echo_serv server(8090, std::thread::hardware_concurrency());
    //async_asio_echo_serv server("127.0.0.1", "8090", 64, std::thread::hardware_concurrency());
    async_asio_echo_serv server(ip, port, packet_size, thread_cnt);
    try
    {
        server.run();

        std::cout << "press [enter] key to continue ...";
        getchar();
        std::cout << std::endl;
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

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

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
