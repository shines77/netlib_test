
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

uint32_t    g_mode = asio_test::mode_need_respond;
std::string g_mode_str = "Need Respond";

asio_test::padding_atomic<uint64_t> asio_test::g_query_count(0);
asio_test::padding_atomic<uint32_t> asio_test::g_client_count(0);

asio_test::padding_atomic<uint64_t> asio_test::g_recieved_bytes(0);
asio_test::padding_atomic<uint64_t> asio_test::g_sent_bytes(0);

using namespace asio_test;

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
            auto cur_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            std::cout << ip.c_str() << ":" << port.c_str() << " - " << packet_size << " B : "
                      << thread_cnt << " thread : "
                      << "[" << client_count << "] conn : qps = "
                      << (cur_succeed_count - last_query_count) << std::endl;
            last_query_count = cur_succeed_count;
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
            auto cur_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            auto qps = (cur_succeed_count - last_query_count);
            std::cout << ip.c_str() << ":" << port.c_str() << " - " << packet_size << " B : "
                      << thread_cnt << " thread : "
                      << "[" << client_count << "] conn : qps = " << qps
                      << std::left << std::setw(5)
                      << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                      << ", BandWidth = " << ((qps * packet_size) / (1024.0 * 1024.0))
                      << " MB/Sec" << std::endl;
            last_query_count = cur_succeed_count;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        server.join();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void print_usage(const std::string & app_name)
{
    std::cerr << std::endl
              << "    Usage: " << app_name.c_str() << " <ip> <port> [<packet_size> = 64] [<thread_cnt> = hw_cpu_cores]" << std::endl
              << std::endl
              << "       For example: " << app_name.c_str() << " 192.168.2.154 8090 64 8" << std::endl;
}

int main(int argc, char * argv[])
{
    std::string app_name;
    app_name = get_app_name(argv[0]);

    int has_respond;
    if ((argc > 1) && (std::strncmp(argv[1], "response=", sizeof("response=") - 1) == 0))
        has_respond = 1;
    else
        has_respond = 0;

    if (argc < 3) {
        print_usage(app_name);
        exit(1);
    }

    std::string ip, port, mode, cmd, cmd_value;
    uint32_t packet_size = 0, thread_cnt = 0;

    g_mode = mode_need_respond;
    g_mode_str = "Need Respond";
    if (has_respond == 1) {
        cmd = argv[1];
        bool succeed = get_cmd_value(cmd, '=', mode);
        if (succeed) {
            if (mode == "0") {
                g_mode = mode_no_respond;
                g_mode_str = "No Respond";
            }
            else {
                g_mode = mode_need_respond;
                g_mode_str = "Need Respond";
            }
        }
    }

    if (argc > (1 + has_respond)) {
        ip = argv[1 + has_respond];
        if (!is_valid_ip_v4(ip)) {
            std::cerr << "Error: ip address \"" << argv[1] << "\" format is wrong." << std::endl;
            exit(1);
        }
    }
    else {
        ip = "127.0.0.1";
    }

    if (argc > (2 + has_respond)) {
        port = argv[2 + has_respond];
        if (!is_socket_port(port)) {
            std::cerr << "Error: port [" << argv[1] << "] number must be range in (0, 65535]." << std::endl;
            exit(1);
        }
    }
    else {
        port = "8090";
    }

    if (argc > (3 + has_respond))
        packet_size = std::atoi(argv[3 + has_respond]);
    if (packet_size <= 0)
        packet_size = 64;
    if (packet_size > MAX_PACKET_SIZE) {
        std::cerr << "Warnning: packet_size = " << packet_size << " is more than "
                  << MAX_PACKET_SIZE << " bytes [MAX_PACKET_SIZE]." << std::endl;
        packet_size = MAX_PACKET_SIZE;
    }

    if (argc > (4 + has_respond))
        thread_cnt = std::atoi(argv[4 + has_respond]);
    if (thread_cnt <= 0)
        thread_cnt = std::thread::hardware_concurrency();

    std::cout << app_name.c_str() << " begin ..." << std::endl;
    std::cout << std::endl;
    std::cout << "listen " << ip.c_str() << ":" << port.c_str() 
              << "  mode: " << g_mode_str.c_str() << std::endl;
    std::cout << "packet_size: " << packet_size << ", thread_cnt: " << thread_cnt << std::endl;
    std::cout << std::endl;

    //run_asio_echo_serv(ip, port, packet_size, thread_cnt);
    run_asio_echo_serv_ex(ip, port, packet_size, thread_cnt);

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
