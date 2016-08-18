
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <exception>
#include <boost/program_options.hpp>

#include "common.h"
#include "async_asio_echo_serv.hpp"
#include "async_aiso_echo_serv_ex.hpp"
#include "common/cmd_utils.hpp"

namespace app_opts = boost::program_options;

uint32_t g_test_mode        = asio_test::mode_need_echo;
uint32_t g_test_category    = asio_test::mode_need_echo;

std::string g_test_mode_str     = "Need Echo";
std::string g_test_category_str = "";
std::string g_rpc_topic;

std::string g_server_ip;
std::string g_server_port;

asio_test::padding_atomic<uint64_t> asio_test::g_query_count(0);
asio_test::padding_atomic<uint32_t> asio_test::g_client_count(0);

asio_test::padding_atomic<uint64_t> asio_test::g_recieved_bytes(0);
asio_test::padding_atomic<uint64_t> asio_test::g_sent_bytes(0);

using namespace asio_test;

void run_asio_echo_serv(const std::string & ip, const std::string & port,
                        uint32_t packet_size, uint32_t thread_num,
                        bool confirm = false)
{
    try
    {
        async_asio_echo_serv server(ip, port, packet_size, thread_num);
        server.run();

        std::cout << "Server has bind and listening ..." << std::endl;
        if (confirm) {
            std::cout << "press [enter] key to continue ...";
            getchar();
        }
        std::cout << std::endl;

        uint64_t last_query_count = 0;
        while (true) {
            auto cur_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            auto qps = (cur_succeed_count - last_query_count);
            std::cout << ip.c_str() << ":" << port.c_str() << " - " << packet_size << " bytes : "
                      << thread_num << " thread(s) : "
                      << "[" << client_count << "] conn(s) : "
                      << "qps = " << std::right << std::setw(8) << qps << ", "
                      << "BandWidth = "
                      << std::right << std::setw(8)
                      << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                      << ((qps * packet_size) / (1024.0 * 1024.0))
                      << " MB/s" << std::endl;
            std::cout << std::right;
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
                           uint32_t packet_size, uint32_t thread_num,
                           bool confirm = false)
{
    static const uint32_t kSeesionBufferSize = 32768;
    try
    {
        async_asio_echo_serv_ex server(ip, port, kSeesionBufferSize, packet_size, thread_num);
        server.run();

        std::cout << "Server has bind and listening ..." << std::endl;
        if (confirm) {
            std::cout << "press [enter] key to continue ...";
            getchar();
        }
        std::cout << std::endl;

        uint64_t last_query_count = 0;
        while (true) {
            auto cur_succeed_count = (uint64_t)g_query_count;
            auto client_count = (uint32_t)g_client_count;
            auto qps = (cur_succeed_count - last_query_count);
            std::cout << ip.c_str() << ":" << port.c_str() << " - " << packet_size << " bytes : "
                      << thread_num << " thread(s) : "
                      << "[" << client_count << "] conn(s) : "
                      << "qps = " << std::right << std::setw(8) << qps << ", "
                      << "BandWidth = "
                      << std::right << std::setw(8)
                      << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                      << ((qps * packet_size) / (1024.0 * 1024.0))
                      << " MB/s" << std::endl;
            std::cout << std::right;
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

void print_usage(const std::string & app_name, const app_opts::options_description & options_desc)
{
    std::cerr << options_desc << std::endl;

    std::cerr << "Usage: " << std::endl
              << "  " << app_name.c_str() << " --mode=<mode> --host=<host> --port=<port> [--packet_size=<packet_size>] [--thread-num=<thread_num>]" << std::endl
              << std::endl
              << "For example: " << std::endl
              << "  " << app_name.c_str() << " --mode=qps --host=127.0.0.1 --port=9000 --packet-size=64 --thread-num=8" << std::endl
              << "  " << app_name.c_str() << " -m qps -s 127.0.0.1 -p 9000 -k 64 -n 8" << std::endl;
}

int main(int argc, char * argv[])
{
    std::string app_name, test_category, test_mode, rpc_topic;
    std::string server_ip, server_port;
    std::string mode, test, cmd, cmd_value;
    uint32_t need_echo = 1, packet_size = 0, thread_num = 0;

    app_name = get_app_name(argv[0]);

    app_opts::options_description desc("Command list");
    desc.add_options()
        ("help,h",                                                                                      "usage info")
        ("mode,m",          app_opts::value<std::string>(&test_mode)->default_value("qps"),             "test mode = [pingpong, qps, latency, throughput]")
        //("test,t",          app_opts::value<std::string>(&test_category)->default_value("echo"),        "test category")
        ("host,s",          app_opts::value<std::string>(&server_ip)->default_value("127.0.0.1"),       "server host or ip address")
        ("port,p",          app_opts::value<std::string>(&server_port)->default_value("9000"),          "server port")
        ("packet-size,k",   app_opts::value<uint32_t>(&packet_size)->default_value(64),                 "packet size")
        ("thread-num,n",    app_opts::value<uint32_t>(&thread_num)->default_value(0),                   "thread numbers")
        //("topic,r",         app_opts::value<std::string>(&rpc_topic)->default_value("add"),             "rpc call's topic")
        ("echo,e",          app_opts::value<uint32_t>(&need_echo)->default_value(1),                    "whether the server need echo")
        ;

    app_opts::variables_map vars_map;
    try {
        app_opts::store(app_opts::parse_command_line(argc, argv, desc), vars_map);
    }
    catch (const std::exception & e) {
        std::cout << "Exception is: " << e.what() << std::endl;
    }
    app_opts::notify(vars_map);

    if (vars_map.count("help") > 0) {
        print_usage(app_name, desc);
        exit(EXIT_FAILURE);
    }

    if (vars_map.count("host") > 0) {
        server_ip = vars_map["host"].as<std::string>();
        std::cout << "host: " << vars_map["host"].as<std::string>().c_str() << std::endl;
        if (!is_valid_ip_v4(server_ip)) {
            std::cerr << "Error: ip address \"" << server_ip.c_str() << "\" format is wrong." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (vars_map.count("port") > 0) {
        server_port = vars_map["port"].as<std::string>();
        std::cout << "port: " << vars_map["port"].as<std::string>().c_str() << std::endl;
        if (!is_socket_port(server_port)) {
            std::cerr << "Error: port [" << server_port.c_str() << "] number must be range in (0, 65535]." << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    if (vars_map.count("packet-size") > 0) {
        packet_size = vars_map["packet-size"].as<uint32_t>();
        std::cout << "packet-size: " << vars_map["packet-size"].as<uint32_t>() << std::endl;
        if (packet_size <= 0)
            packet_size = MIN_PACKET_SIZE;
        if (packet_size > MAX_PACKET_SIZE) {
            packet_size = MAX_PACKET_SIZE;
            std::cerr << "Warnning: packet_size = " << packet_size << " can not set to more than "
                      << MAX_PACKET_SIZE << " bytes [MAX_PACKET_SIZE]." << std::endl;
        }
    }

    if (vars_map.count("thread-num") > 0) {
        thread_num = vars_map["thread-num"].as<uint32_t>();
        std::cout << "thread-num: " << vars_map["thread-num"].as<uint32_t>() << std::endl;
        if (thread_num <= 0) {
            thread_num = std::thread::hardware_concurrency();
            std::cout << ">>> thread-num: std::thread::hardware_concurrency() = " << thread_num << std::endl;
        }
    }

    if (vars_map.count("mode") > 0) {
        mode = vars_map["mode"].as<std::string>();
        std::cout << "test mode: " << vars_map["mode"].as<std::string>().c_str() << std::endl;
    }

    if (vars_map.count("test") > 0) {
        test = vars_map["test"].as<std::string>();
        std::cout << "test category: " << vars_map["test"].as<std::string>().c_str() << std::endl;
    }

    need_echo = 1;
    g_test_mode = mode_need_echo;
    g_test_mode_str = "Need Echo";

    if (vars_map.count("echo") > 0) {
        need_echo = vars_map["echo"].as<uint32_t>();
        std::cout << "need_echo: " << vars_map["echo"].as<uint32_t>() << std::endl;
        if (need_echo == 0) {
            g_test_mode = mode_dont_need_echo;
            g_test_mode_str = "Don't Need Echo";
        }
        else {
            g_test_mode = mode_need_echo;
            g_test_mode_str = "Need Echo";
        }
    }
    else {
        std::cout << "need_echo: " << need_echo << std::endl;
    }

    std::cout << std::endl;
    std::cout << app_name.c_str() << " begin ..." << std::endl;
    std::cout << std::endl;
    std::cout << "listen " << server_ip.c_str() << ":" << server_port.c_str() << std::endl;
    std::cout << "mode: " << g_test_mode_str.c_str() << std::endl;
    std::cout << "packet_size: " << packet_size << ", thread_num: " << thread_num << std::endl;
    std::cout << std::endl;

    //run_asio_echo_serv(server_ip, server_port, packet_size, thread_num);
    run_asio_echo_serv_ex(server_ip, server_port, packet_size, thread_num);

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
