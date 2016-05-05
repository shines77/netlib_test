
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
#include <boost/asio.hpp>

#include "basic_client.hpp"

using namespace boost::asio;

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

void run_client(const std::string & app, const std::string & ip,
    const std::string & port, std::uint32_t packet_size)
{
    std::cout << app.c_str() << " begin." << std::endl;
    std::cout << std::endl;
    try
    {
        boost::asio::io_service io_service;

        ip::tcp::resolver resolver(io_service);
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.154", "8090"} );
        //auto endpoint_iterator = resolver.resolve( {"192.168.2.191", "8090"} );
        auto endpoint_iterator = resolver.resolve( { ip, port } );
        echo_client client(io_service, endpoint_iterator, packet_size);

        std::cout << "connectting " << ip.c_str() << ":" << port.c_str() << std::endl;
        std::cout << "packet_size: " << packet_size << std::endl;
        std::cout << std::endl;

        io_service.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    std::cout << app.c_str() << " done." << std::endl;
}

int main(int argc, char * argv[])
{
    if (argc <= 2) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> [<packet_size> = 64]" << std::endl;
        return 1;
    }

    std::string app, ip, port;
    uint32_t packet_size = 0, thread_cnt = 0;

    app = argv[0];

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

    run_client(app, ip, port, packet_size);

    uint32_t loop_times = 0;

    while (true) {
        std::cout << loop_times << std::endl;
        loop_times++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    ::system("pause");
    return 0;
}
