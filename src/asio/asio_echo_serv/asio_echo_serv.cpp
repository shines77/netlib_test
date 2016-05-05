
#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS)
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
#define _WIN32_WINNT	0x0501
#endif
#endif

#ifdef _MSC_VER
#define BOOST_ASIO_MSVC _MSC_VER
#endif

# if defined(BOOST_ASIO_MSVC)
#  if (_MSC_VER >= 1900)
#   define BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT noexcept(true)
#  endif // (_MSC_VER >= 1900)
# endif // defined(BOOST_ASIO_MSVC)

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>

#include "async_asio_echo_serv.hpp"

using namespace asio_test;

std::atomic<uint64_t> g_query_count = 0;
std::atomic<uint32_t> g_client_count = 0;

bool is_valid_ip_v4(const std::string & ip)
{
    if (ip.empty() || ip.length() > 15)
        return false;

    int dot_cnt = 0;
    int num_cnt = 0;
    std::string::const_iterator iter;
    for (iter = ip.begin(); iter != ip.end(); ++iter) {
        char ch = *iter;
        if ((ch < '0' || ch > '9') && (ch != '.'))
            return false;
        if (ch >= '0' && ch <= '9') {
            num_cnt++;
        }
        else if (ch == '.') {
            if (num_cnt <= 0 || num_cnt > 3)
                return false;
            dot_cnt++;
            num_cnt = 0;
        }
    }
    return (dot_cnt == 3);
}

bool is_number(const std::string & str)
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
    if (!is_number(port))
        port = "8090";

    if (argc > 3)
        packet_size = atoi(argv[3]);
    if (packet_size <= 0)
        packet_size = 64;
    if (packet_size > MAX_PACKET_SIZE)
        packet_size = MAX_PACKET_SIZE;

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

        std::cout << "Press [Enter] key to continue ...";
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
        std::cout << "[" << client_count << "] " << packet_size << " " << (curr_succeed_count - last_query_count) << std::endl;
        last_query_count = curr_succeed_count;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    server.join();

    ::system("pause");
    return 0;
}
