
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>

#include "common.h"
#include "async_asio_echo_serv.hpp"
#include "async_aiso_echo_serv_ex.hpp"
#include "common/cmd_utils.hpp"

std::atomic<uint64_t> g_query_count_(0);
std::atomic<uint32_t> g_client_count_(0);

asio_test::padding_atomic<uint64_t> asio_test::g_query_count(0);
asio_test::padding_atomic<uint32_t> asio_test::g_client_count(0);

asio_test::padding_atomic<uint64_t> asio_test::g_recieved_bytes(0);
asio_test::padding_atomic<uint64_t> asio_test::g_sent_bytes(0);

using namespace asio_test;

void print_usage(const std::string & app_name)
{
    std::cerr << "Usage: " << app_name.c_str() << " <ip> <port> [<packet_size> = 64] [<thread_cnt> = hw_cpu_cores]" << std::endl << std::endl
              << "       For example: " << app_name.c_str() << " 192.168.2.154 8090 64 8" << std::endl;
}

void run_asio_echo_serv(const std::string & ip, const std::string & port,
                        uint32_t packet_size, uint32_t thread_cnt)
{
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

        uint64_t last_query_count = 0;
        while (true) {
            auto curr_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            std::cout << ip.c_str() << ":" << port.c_str() << " - [" << client_count << "] conn - "
                      << thread_cnt << " thread - " << packet_size << " B - qps = "
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
}

void run_asio_echo_serv_ex(const std::string & ip, const std::string & port,
                           uint32_t packet_size, uint32_t thread_cnt)
{
    static const uint32_t kSeesionBufferSize = 32768;
    try
    {
        async_asio_echo_serv_ex server(ip, port, kSeesionBufferSize, packet_size, thread_cnt);
        server.run();

        std::cout << "Server has bind and listening ..." << std::endl;
        //std::cout << "press [enter] key to continue ...";
        //getchar();
        std::cout << std::endl;

        uint64_t last_query_count = 0;
        while (true) {
            auto curr_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            std::cout << ip.c_str() << ":" << port.c_str() << " - [" << client_count << "] conn - "
                      << thread_cnt << " thread - " << packet_size << " B - qps = "
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

    //run_asio_echo_serv(ip, port, packet_size, thread_cnt);
    run_asio_echo_serv_ex(ip, port, packet_size, thread_cnt);

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
