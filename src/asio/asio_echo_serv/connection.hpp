
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include "common.h"

using namespace boost::system;

#define MAX_PACKET_SIZE	8192

using namespace boost::asio;

namespace asio_test {

class connection : public std::enable_shared_from_this<connection>,
                   private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    ip::tcp::socket socket_;
    std::uint32_t packet_size_;
    std::uint64_t query_count_;

    char data_[PACKET_SIZE];

public:
    connection(boost::asio::io_service & io_service, std::uint32_t packet_size)
        : socket_(io_service), packet_size_(packet_size), query_count_(0)
    {
        ::memset(data_, 0, sizeof(data_));
    }

    ~connection()
    {
        //
    }

    void start()
    {
        g_client_count++;
        do_read();
    }

    void stop()
    {
        //
    }

    ip::tcp::socket & socket()
    {
        return socket_;
    }

    static boost::shared_ptr<connection> create_new(boost::asio::io_service & io_service, std::uint32_t packet_size) {
        return boost::shared_ptr<connection>(new connection(io_service, packet_size));
    }

private:
    void do_read()
    {
        //auto self(this->shared_from_this());
#if 0
        boost::asio::async_read(socket_, boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "connection::do_read() - Error: (code = " << ec.value() << ") "
                        << ec.message().c_str() << std::endl;
                    g_client_count--;
                    return;
                }
            }
        );
#else
        socket_.async_read_some(boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "connection::do_read() - Error: (code = " << ec.value() << ") "
                        << ec.message().c_str() << std::endl;
                    g_client_count--;
                    return;
                }
            }
        );
#endif
    }

    void do_write()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec) {
                    do_read();
                    // If get a circle of ping-pong, we count the query one time.
#if 0
                    g_query_count++;
#else
                    query_count_++;
                    if ((query_count_ & 0x00FF) == 0x00FF) {
                        g_query_count += 0x00FF;
                        query_count_ = 0;
                    }
#endif
                }
                else {
                    // Write error log
                    std::cout << "connection::do_write() - Error: (code = " << ec.value() << ") "
                        << ec.message().c_str() << std::endl;
                    g_client_count--;
                    return;
                }
            }
        );
    }
};

} // namespace asio_test
