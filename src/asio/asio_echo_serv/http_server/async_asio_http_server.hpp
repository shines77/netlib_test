
#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/signal_set.hpp>

#include "../common.h"
#include "../io_service_pool.hpp"
#include "asio_http_session.hpp"

using namespace boost::asio;

namespace asio_test {

//
// See: http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp
//
class async_asio_http_server : public std::enable_shared_from_this<async_asio_http_server>,
                               private boost::noncopyable
{
private:
    io_service_pool					    io_service_pool_;
    connection_manager                  connection_manager_;
    boost::asio::ip::tcp::acceptor	    acceptor_;
    std::shared_ptr<asio_http_session>	session_;
    std::shared_ptr<std::thread>	    thread_;
    uint32_t                            buffer_size_;
    uint32_t					        packet_size_;

public:
    async_asio_http_server(const std::string & ip_addr, const std::string & port,
        uint32_t buffer_size = 65536 * 2,
        uint32_t packet_size = 64,
        uint32_t pool_size = std::thread::hardware_concurrency())
        : io_service_pool_(pool_size), acceptor_(io_service_pool_.get_first_io_service()),
          buffer_size_(buffer_size), packet_size_(packet_size)
    {
#if defined(__linux__)
        //
        // See: https://codeday.me/bug/20181108/361396.html
        //
        // boost::asio::signal_set, Since boost 1.5.3
        //
        // Register signal handlers so that the daemon may be shut down. You may
        // also want to register for other signals, such as SIGHUP to trigger a
        // re-read of a configuration file.
        //
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
        signals.async_wait([this](const boost::system::error_code & ec, int signal_no)
                            {
                                io_service_pool_.stop();
                            });
#endif
        start(ip_addr, port);
    }

    async_asio_http_server(short port, uint32_t buffer_size = 65536,
        uint32_t packet_size = 64,
        uint32_t pool_size = std::thread::hardware_concurrency())
        : io_service_pool_(pool_size),
          acceptor_(io_service_pool_.get_first_io_service(), ip::tcp::endpoint(ip::tcp::v4(), port)),
          buffer_size_(buffer_size), packet_size_(packet_size)
    {
        do_accept();
    }

    ~async_asio_http_server()
    {
        connection_manager_.stop_all();
        this->stop();
    }

    void start(const std::string & ip_addr, const std::string & port)
    {
        ip::tcp::resolver resolver(io_service_pool_.get_now_io_service());
        ip::tcp::resolver::query query(ip_addr, port);
        ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            // Open endpoint error
            std::cout << "async_asio_http_server::start() - Error: (code = " << ec.value() << ") "
                      << ec.message().c_str() << std::endl;
            return;
        }

        boost::asio::socket_base::reuse_address option(true);
        acceptor_.set_option(option);
        acceptor_.bind(endpoint);
        acceptor_.listen();

        do_accept();
    }

    void stop()
    {
        acceptor_.cancel();
    }

    void run()
    {
        thread_ = std::make_shared<std::thread>([this] { io_service_pool_.run(); });
    }

    void join()
    {
        if (thread_->joinable())
            thread_->join();
    }

private:
    void handle_accept(const boost::system::error_code & ec, asio_http_session * session)
    {
        if (!ec) {
            if (session) {
                session->start();
            }
            do_accept();
        }
        else {
            // Accept error
            std::cout << "async_asio_http_server::handle_accept() - Error: (code = " << ec.value() << ") "
                      << ec.message().c_str() << std::endl;
            if (session) {
                session->stop();
                delete session;
            }
        }        
    }

    void do_accept()
    {
        asio_http_session * new_session = new asio_http_session(io_service_pool_.get_io_service(),
                                                                &connection_manager_, buffer_size_, packet_size_, g_test_mode);
        acceptor_.async_accept(new_session->socket(), boost::bind(&async_asio_http_server::handle_accept,
                               this, boost::asio::placeholders::error, new_session));
    }

    void do_accept_lambda()
    {
        session_.reset(new asio_http_session(io_service_pool_.get_io_service(), &connection_manager_,
                                             buffer_size_, packet_size_, g_test_mode));
        acceptor_.async_accept(session_->socket(),
            [this](const boost::system::error_code & ec)
            {
                if (!ec) {
                    session_->start();
                }
                else {
                    // Accept error
                    std::cout << "async_asio_http_server::handle_accept_lambda() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    session_->stop();
                    session_.reset();
                }

                do_accept_lambda();
            });
    }
};

} // namespace asio_test
