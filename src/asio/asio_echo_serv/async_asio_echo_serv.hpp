
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

#include "common.h"
#include "io_service_pool.hpp"
#include "asio_connection.hpp"

using namespace boost::asio;

namespace asio_test {

//
// See: http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp
//
class async_asio_echo_serv : public boost::enable_shared_from_this<async_asio_echo_serv>,
                             private boost::noncopyable
{
private:
    io_service_pool					    io_service_pool_;
    boost::asio::ip::tcp::acceptor	    acceptor_;
    std::shared_ptr<asio_connection>    conn_;
    std::shared_ptr<std::thread>	    thread_;
    uint32_t					        packet_size_;

public:
    async_asio_echo_serv(const std::string & ip_addr, const std::string & port,
        uint32_t packet_size = 64,
        uint32_t pool_size = std::thread::hardware_concurrency())
        : io_service_pool_(pool_size), acceptor_(io_service_pool_.get_first_io_service()),
          packet_size_(packet_size)
    {
        start(ip_addr, port);
    }

    async_asio_echo_serv(short port, uint32_t packet_size = 64,
        uint32_t pool_size = std::thread::hardware_concurrency())
        : io_service_pool_(pool_size), acceptor_(io_service_pool_.get_first_io_service(),
          ip::tcp::endpoint(ip::tcp::v4(), port)),
          packet_size_(packet_size)
    {
        do_accept();
    }

    ~async_asio_echo_serv()
    {
        this->stop();
    }

    void start(const std::string & ip_addr, const std::string & port)
    {
        ip::tcp::resolver resolver(io_service_pool_.get_now_io_service());
        ip::tcp::resolver::query query(ip_addr, port);
        ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        do_accept();
    }

    void stop()
    {
        if (acceptor_.is_open()) {
            acceptor_.cancel();
            acceptor_.close();
        }
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
    void handle_accept(const boost::system::error_code & ec, asio_connection * conn)
    {
        if (!ec) {
            if (conn) {
                conn->start();
            }
        }
        else {
            // Accept error
            std::cout << "async_asio_echo_serv::handle_accept() - Error: (code = " << ec.value() << ") "
                      << ec.message().c_str() << std::endl;
            if (conn) {
                conn->stop();
                delete conn;
            }
        }

        do_accept();
    }

    void do_accept()
    {
        asio_connection * new_conn = new asio_connection(io_service_pool_.get_io_service(), packet_size_);
        acceptor_.async_accept(new_conn->socket(), boost::bind(&async_asio_echo_serv::handle_accept,
                               this, boost::asio::placeholders::error, new_conn));
    }

    void do_accept_lambda()
    {
        conn_.reset(new asio_connection(io_service_pool_.get_io_service(), packet_size_));
        acceptor_.async_accept(conn_->socket(),
            [this](boost::system::error_code ec)
            {
                if (!ec) {
                    conn_->start();
                }
                else {
                    // Accept error
                    std::cout << "async_asio_echo_serv::handle_accept_lambda() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    conn_->stop();
                    conn_.reset();
                }

                do_accept_lambda();
            });
    }
};

} // namespace asio_test
