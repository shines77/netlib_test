
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "common.h"
#include "echo_client.hpp"
#include "test_latency_client.hpp"
#include "common/cmd_utils.hpp"

using namespace boost::asio;
using namespace asio_test;

uint32_t g_mode = mode_pingpong;
std::string   g_mode_str = "pingpong";

void run_pingpong_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size)
{
    std::cout << app_name.c_str() << " mode = " << g_mode_str.c_str() << std::endl;
    std::cout << std::endl;
    try
    {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.154", "8090"} );
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        echo_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_qps_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size)
{
    std::cout << app_name.c_str() << " mode = " << g_mode_str.c_str() << std::endl;
    std::cout << std::endl;
    try
    {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.154", "8090"} );
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        echo_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_latency_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size)
{
    std::cout << app_name.c_str() << " mode = " << g_mode_str.c_str() << std::endl;
    std::cout << std::endl;
    try
    {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.154", "8090"} );
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        test_latency_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void run_throughout_client(const std::string & app_name, const std::string & ip,
    const std::string & port, uint32_t packet_size)
{
    std::cout << app_name.c_str() << " mode = " << g_mode_str.c_str() << std::endl;
    std::cout << std::endl;
    try
    {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.154", "8090"} );
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        echo_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    std::cout << app_name.c_str() << " done." << std::endl;
}

void print_usage(const std::string & app_name)
{
    std::cerr << "Usage: " << app_name.c_str() << " <mode=xxxx> <ip> <port> [<packet_size> = 64]" << std::endl
              << "       mode:        Client run mode, you can choose pingpong, qps, latency and throughout." << std::endl << std::endl
              << "       For example: " << app_name.c_str() << " mode=pingpong 192.168.2.154 8090 64" << std::endl;
}

int main(int argc, char * argv[])
{
    std::string app_name;
    app_name = get_app_name(argv[0]);

    int has_mode;
    if (argc >= 2 && std::strncmp(argv[1], "mode=", sizeof("mode=") - 1) == 0)
        has_mode = 1;
    else
        has_mode = 0;

    if (argc <= (2 + has_mode) || (argc >= 2
        && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0))) {
        print_usage(app_name);
        return 1;
    }

    std::string ip, port, cmd, mode;
    uint32_t packet_size = 0;

    if (has_mode == 1) {
        cmd = argv[1];
        g_mode = mode_unknown;
        bool succeed = get_cmd_value(cmd, '=', mode);
        if (succeed) {
            g_mode_str = mode;
            if (mode == "pingpong")
                g_mode = mode_pingpong;
            else if (mode == "qps")
                g_mode = mode_qps;
            else if (mode == "latency")
                g_mode = mode_latency;
            else if (mode == "throughout")
                g_mode = mode_throughout;
            else {
                // Write error log: Unknown mode
                std::cerr << "Error: Unknown mode [" << mode.c_str() << "]." << std::endl;
                return 1;
            }
        }
    }

    ip = argv[1 + has_mode];
    if (!is_valid_ip_v4(ip)) {
        // ip address format wrong
        std::cerr << "Error: ip address \"" << argv[1 + has_mode] << "\" format is wrong." << std::endl;
        return 1;
    }

    port = argv[2 + has_mode];
    if (!is_socket_port(port)) {
        // port is not correct format
        std::cerr << "Error: port [" << argv[2 + has_mode] << "] number must be range in (0, 65535]." << std::endl;
        return 1;
    }

    if (argc > (3 + has_mode))
        packet_size = atoi(argv[3 + has_mode]);
    if (packet_size <= 0)
        packet_size = 64;
    if (packet_size > MAX_PACKET_SIZE) {
        std::cerr << "Warnning: packet_size = " << packet_size << " is more than "
                  << MAX_PACKET_SIZE << " bytes [MAX_PACKET_SIZE]." << std::endl;
        packet_size = MAX_PACKET_SIZE;
    }

    if (g_mode == mode_pingpong)
        run_pingpong_client(app_name, ip, port, packet_size);
    else if (g_mode == mode_qps)
        run_qps_client(app_name, ip, port, packet_size);
    else if (g_mode == mode_latency)
        run_latency_client(app_name, ip, port, packet_size);
    else if (g_mode == mode_throughout)
        run_throughout_client(app_name, ip, port, packet_size);
    else {
        // Write error log.
    }

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
