
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

uint32_t g_test_mode      = asio_test::test_mode_echo;
uint32_t g_test_method    = asio_test::test_method_pingpong;

std::string g_test_mode_str     = "pingpong";
std::string g_test_method_str   = "";

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

void make_align_spaces(const std::string & name, std::string & spaces)
{
    size_t len = name.size();
    spaces = "";
    for (size_t i = 0; i < len; ++i)
        spaces += " ";
}

void print_usage(const std::string & app_name, const app_opts::options_description & options_desc)
{
    std::string align_spaces;
    make_align_spaces(app_name, align_spaces);

    std::cerr << std::endl;
    std::cerr << options_desc << std::endl;

    std::cerr << "Usage: " << std::endl << std::endl
              << "  " << app_name.c_str()     << " --host=<host> --port=<port> --mode=<mode> --test=<test>" << std::endl
              << "  " << align_spaces.c_str() << " --pipeline=<pipeline> [--packet_size=64] [--thread-num=0]" << std::endl
              << std::endl
              << "For example: " << std::endl << std::endl
              << "  " << app_name.c_str()     << " --host=127.0.0.1 --port=9000 --mode=echo --test=pingpong" << std::endl
              << "  " << align_spaces.c_str() << " --pipeline=10 --packet-size=64 --thread-num=8" << std::endl
              << std::endl
              << "  " << app_name.c_str() << " -s 127.0.0.1 -p 9000 -m echo -t pingpong -l 10 -k 64 -n 8" << std::endl;
    std::cerr << std::endl;
}

int main(int argc, char * argv[])
{
    std::string app_name, test_mode, test_method, rpc_topic;
    std::string server_ip, server_port;
    std::string mode, test, cmd, cmd_value;
    int32_t pipeline = 1, packet_size = 0, thread_num = 0, test_time = 30, need_echo = 1;

    app_name = get_app_name(argv[0]);

    app_opts::options_description desc("Command list");
    desc.add_options()
        ("help,h",                                                                                      "usage info")
        ("host,s",          app_opts::value<std::string>(&server_ip)->default_value("127.0.0.1"),       "server host or ip address")
        ("port,p",          app_opts::value<std::string>(&server_port)->default_value("9000"),          "server port")
        ("mode,m",          app_opts::value<std::string>(&test_mode)->default_value("echo"),            "test mode = [echo]")
        ("test,t",          app_opts::value<std::string>(&test_method)->default_value("pingpong"),      "test method = [pingpong, qps, latency, throughput]")
        ("pipeline,l",      app_opts::value<int32_t>(&pipeline)->default_value(1),                      "pipeline numbers")
        ("packet-size,k",   app_opts::value<int32_t>(&packet_size)->default_value(64),                  "packet size")
        ("thread-num,n",    app_opts::value<int32_t>(&thread_num)->default_value(1),                    "thread numbers")
        ("test-time,i",     app_opts::value<int32_t>(&test_time)->default_value(30),                    "total test time (seconds)")
        ("echo,e",          app_opts::value<int32_t>(&need_echo)->default_value(1),                     "whether the server need echo")
        ;

    // parse command line
    app_opts::variables_map vars_map;
    try {
        app_opts::store(app_opts::parse_command_line(argc, argv, desc), vars_map);
    }
    catch (const std::exception & e) {
        std::cout << "Exception is: " << e.what() << std::endl;
    }
    app_opts::notify(vars_map);

    // help
    if (vars_map.count("help") > 0) {
        print_usage(app_name, desc);
        exit(EXIT_FAILURE);
    }

    // host
    if (vars_map.count("host") > 0) {
        server_ip = vars_map["host"].as<std::string>();
    }
    std::cout << "host: " << server_ip.c_str() << std::endl;
    if (!is_valid_ip_v4(server_ip)) {
        std::cerr << "Error: ip address \"" << server_ip.c_str() << "\" format is wrong." << std::endl;
        exit(EXIT_FAILURE);
    }

    // port
    if (vars_map.count("port") > 0) {
        server_port = vars_map["port"].as<std::string>();
    }
    std::cout << "port: " << server_port.c_str() << std::endl;
    if (!is_socket_port(server_port)) {
        std::cerr << "Error: port [" << server_port.c_str() << "] number must be range in (0, 65535]." << std::endl;
        exit(EXIT_FAILURE);
    }

    // mode
    g_test_mode = test_mode_unknown;
    g_test_mode_str = "echo";

    if (vars_map.count("mode") > 0) {
        test_mode = vars_map["mode"].as<std::string>();
        g_test_mode_str = test_mode;
    }
    mode = g_test_mode_str;
    std::cout << "test mode: " << mode.c_str() << std::endl;
    if (mode == "echo") {
        g_test_mode = test_mode_echo;
    }
    else {
        // Write error log: Unknown test mode
        std::cerr << "Error: Unknown test mode: [" << mode.c_str() << "]." << std::endl;
        exit(EXIT_FAILURE);
    }

    // test
    g_test_method = test_method_unknown;
    g_test_method_str = "pingpong";

    if (vars_map.count("test") > 0) {
        test_method = vars_map["test"].as<std::string>();
        g_test_method_str = test_method;
    }
    test = g_test_method_str;
    std::cout << "test mode: " << test.c_str() << std::endl;
    if (test == "pingpong") {
        g_test_method = test_method_pingpong;
    }
    else if (test == "qps") {
        g_test_method = test_method_qps;
    }
    else if (test == "throughput") {
        g_test_method = test_method_throughput;
    }
    else if (test == "latency") {
        g_test_method = test_method_latency;
    }
    else {
        // Write error log: Unknown test method
        std::cerr << "Error: Unknown test method: [" << test.c_str() << "]." << std::endl;
        exit(EXIT_FAILURE);
    }

    // pipeline
    if (vars_map.count("pipeline") > 0) {
        pipeline = vars_map["pipeline"].as<int32_t>();
        std::cout << "pipeline: " << pipeline << std::endl;
    }
    if (pipeline <= 0)
        pipeline = 1;

    // packet-size
    if (vars_map.count("packet-size") > 0) {
        packet_size = vars_map["packet-size"].as<int32_t>();
    }
    std::cout << "packet-size: " << packet_size << std::endl;
    if (packet_size <= 0)
        packet_size = MIN_PACKET_SIZE;
    if (packet_size > MAX_PACKET_SIZE) {
        packet_size = MAX_PACKET_SIZE;
        std::cerr << "Warnning: packet_size = " << packet_size << " can not set to more than "
                  << MAX_PACKET_SIZE << " bytes [MAX_PACKET_SIZE]." << std::endl;
    }

    // thread-num
    if (vars_map.count("thread-num") > 0) {
        thread_num = vars_map["thread-num"].as<int32_t>();
    }
    std::cout << "thread-num: " << thread_num << std::endl;
    if (thread_num <= 0) {
        thread_num = std::thread::hardware_concurrency();
        std::cout << ">>> thread-num: std::thread::hardware_concurrency() = " << thread_num << std::endl;
    }

    // test-time
    if (vars_map.count("test-time") > 0) {
        test_time = vars_map["test-time"].as<int32_t>();
    }
    std::cout << "test-time: " << test_time << std::endl;
    if (test_time <= 0) {
        test_time = 30;
        std::cout << ">>> test-time: " << test_time << std::endl;
    }

    // need_echo = 1;
    if (vars_map.count("echo") > 0) {
        need_echo = vars_map["echo"].as<int32_t>();
    }
    std::cout << "need_echo: " << need_echo << std::endl;

    // Run a test method
    if (g_test_method == test_method_pingpong)
        run_pingpong_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_method == test_method_qps)
        run_qps_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_method == test_method_throughput)
        run_throughput_client(app_name, server_ip, server_port, packet_size, test_time);
    else if (g_test_method == test_method_latency)
        run_latency_client(app_name, server_ip, server_port, packet_size, test_time);
    else {
        // Write error log.
        std::cerr << "Error: Unknown test method: [" << g_test_method << "]." << std::endl;
    }

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
