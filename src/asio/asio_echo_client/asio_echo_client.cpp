
#include "boost_asio_msvc.h"

#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "common.h"
#include "echo_client.hpp"
#include "test_latency_client.hpp"

using namespace boost::asio;

std::uint32_t g_mode = mode_pingpong;
std::string   g_mode_str = "pingpong";

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

bool get_cmd_value(const std::string & cmd, char sep, std::string & cmd_value)
{
    std::string::const_iterator iter;
    for (iter = cmd.begin(); iter != cmd.end(); ++iter) {
        char ch = *iter;
        if (ch == sep) {
            ++iter;
            cmd_value = cmd.substr(iter - cmd.begin());
            return true;
        }
    }
    return false;
}

void run_pingpong_client(const std::string & app_name, const std::string & ip,
    const std::string & port, std::uint32_t packet_size)
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
    const std::string & port, std::uint32_t packet_size)
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
    const std::string & port, std::uint32_t packet_size)
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
    const std::string & port, std::uint32_t packet_size)
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
        //ip = "127.0.0.1";
        std::cerr << "Error: ip address \"" << argv[1 + has_mode] << "\" format is wrong." << std::endl;
        return 1;
    }

    port = argv[2 + has_mode];
    if (!is_socket_port(port)) {
        //port = "8090";
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

    uint32_t loop_times = 0;
    while (true) {
        std::cout << loop_times << std::endl;
        loop_times++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

#ifdef _WIN32
    ::system("pause");
#endif
    return 0;
}
