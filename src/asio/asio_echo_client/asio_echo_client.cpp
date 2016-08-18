
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "common.h"
#include "test_pingpong_client.hpp"
#include "test_latency_client.hpp"
#include "test_qps_client.hpp"
#include "common/cmd_utils.hpp"

namespace app_opts = boost::program_options;

uint32_t g_test_mode        = asio_test::mode_pingpong;
uint32_t g_test_category    = asio_test::mode_pingpong;

std::string g_test_mode_str     = "pingpong";
std::string g_test_category_str = "";

std::string g_server_ip;
std::string g_server_port;

using namespace boost::asio;
using namespace asio_test;

void run_pingpong_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size, uint32_t test_time)
{
    std::cout << std::endl;
    std::cout << app_name.c_str() << " [mode = " << g_test_mode_str.c_str() << "]" << std::endl;
    std::cout << std::endl;
    try {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        test_pingpong_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_qps_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size, uint32_t test_time)
{
    std::cout << std::endl;
    std::cout << app_name.c_str() << " [mode = " << g_test_mode_str.c_str() << "]" << std::endl;
    std::cout << std::endl;
    try {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        test_qps_client client(io_service, endpoint_iterator, g_test_mode, 32768, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_throughput_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size, uint32_t test_time)
{
    std::cout << std::endl;
    std::cout << app_name.c_str() << " [mode = " << g_test_mode_str.c_str() << "]" << std::endl;
    std::cout << std::endl;
    try {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        test_qps_client client(io_service, endpoint_iterator, g_test_mode, 32768, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_latency_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size, uint32_t test_time)
{
    std::cout << std::endl;
    std::cout << app_name.c_str() << " [mode = " << g_test_mode_str.c_str() << "]" << std::endl;
    std::cout << std::endl;
    try {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        test_latency_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
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
    uint32_t need_echo = 1, packet_size = 0, thread_num = 0, test_time = 30;

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
        ("test-time,i",     app_opts::value<uint32_t>(&test_time)->default_value(30),                   "total test time (seconds)")
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

    g_test_mode = mode_unknown;
    g_test_mode_str = "pingpong";

    if (vars_map.count("mode") > 0) {
        mode = vars_map["mode"].as<std::string>();
        std::cout << "test mode: " << vars_map["mode"].as<std::string>().c_str() << std::endl;
        g_test_mode_str = mode;
        if (mode == "pingpong")
            g_test_mode = mode_pingpong;
        else if (mode == "qps")
            g_test_mode = mode_qps;
        else if (mode == "throughput")
            g_test_mode = mode_throughput;
        else if (mode == "latency")
            g_test_mode = mode_latency;
        else {
            // Write error log: Unknown test mode
            std::cerr << "Error: Unknown mode: [" << mode.c_str() << "]." << std::endl;
            exit(1);
        }
    }

    //if (vars_map.count("test") > 0) {
    //    test = vars_map["test"].as<std::string>();
    //    std::cout << "test category: " << vars_map["test"].as<std::string>().c_str() << std::endl;
    //}

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

    //if (vars_map.count("thread-num") > 0) {
    //    thread_num = vars_map["thread-num"].as<uint32_t>();
    //    std::cout << "thread-num: " << vars_map["thread-num"].as<uint32_t>() << std::endl;
    //    if (thread_num <= 0) {
    //        thread_num = std::thread::hardware_concurrency();
    //        std::cout << ">>> thread-num: std::thread::hardware_concurrency() = " << thread_num << std::endl;
    //    }
    //}

    if (vars_map.count("test-time") > 0) {
        test_time = vars_map["test-time"].as<uint32_t>();
        std::cout << "test-time: " << vars_map["test-time"].as<uint32_t>() << std::endl;
        if (test_time <= 0) {
            test_time = 30;
            std::cout << ">>> test-time: " << test_time << std::endl;
        }
    }

    need_echo = 1;
    if (vars_map.count("echo") > 0) {
        need_echo = vars_map["echo"].as<uint32_t>();
        std::cout << "need_echo: " << vars_map["echo"].as<uint32_t>() << std::endl;
    }

    if (g_test_mode == mode_pingpong)
        run_pingpong_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_mode == mode_qps)
        run_qps_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_mode == mode_throughput)
        run_throughput_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_mode == mode_latency)
        run_latency_client(app_name, server_ip, server_port, packet_size, test_time);
    else {
        // Write error log.
        std::cerr << "Error: Unknown mode: [" << g_test_mode << "]." << std::endl;
    }

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
