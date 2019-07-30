
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/smart_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include "common.h"

using namespace boost::system;

#define MIN_PACKET_SIZE             64
#define MAX_PACKET_SIZE	            (64 * 1024)

// Whether use atomic update realtime?
#define USE_ATOMIC_REALTIME_UPDATE  0

#define QUERY_COUNTER_INTERVAL      99

using namespace boost::asio;

namespace asio_test {

class asio_connection : public boost::enable_shared_from_this<asio_connection>,
                        private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    ip::tcp::socket socket_;
    uint32_t packet_size_;
    uint64_t query_count_;

    char data_[PACKET_SIZE];

public:
    asio_connection(boost::asio::io_service & io_service, uint32_t packet_size)
        : socket_(io_service), packet_size_(packet_size), query_count_(0)
    {
        ::memset(data_, 'k', sizeof(data_));
    }

    ~asio_connection()
    {
        stop(false);
    }

    void start()
    {
        //set_socket_recv_bufsize(MAX_PACKET_SIZE);

        g_client_count++;

        do_read();
    }

    void stop(bool delete_self = false)
    {
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT >= 0x0600)
        socket_.cancel();
#endif

        //socket_.shutdown(socket_base::shutdown_both);
        if (socket_.is_open()) {
            socket_.close();

            if (g_client_count.load() != 0)
                g_client_count--;
        }

        if (delete_self)
            delete this;
    }

    ip::tcp::socket & socket()
    {
        return socket_;
    }

    static boost::shared_ptr<asio_connection> create_new(
        boost::asio::io_service & io_service, uint32_t packet_size) {
        return boost::shared_ptr<asio_connection>(new asio_connection(io_service, packet_size));
    }

private:
    int get_socket_recv_bufsize() const
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option;
        socket_.get_option(recv_bufsize_option);

        //std::cout << "receive_buffer_size: " << recv_bufsize_option.value() << " bytes" << std::endl;
        return recv_bufsize_option.value();
    }

    void set_socket_recv_bufsize(int buffer_size)
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option(buffer_size);
        socket_.set_option(recv_bufsize_option);

        //std::cout << "set_socket_recv_buffer_size(): " << buffer_size << " bytes" << std::endl;
    }

    inline void do_query_counter()
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        g_query_count.fetch_add(1);
#else
        query_count_++;
        if (query_count_ >= QUERY_COUNTER_INTERVAL) {
            g_query_count.fetch_add(QUERY_COUNTER_INTERVAL);
            query_count_ = 0;
        }
#endif
    }

    void do_read()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t received_bytes)
            {
                if ((uint32_t)received_bytes != packet_size_) {
                    std::cout << "asio_connection::do_read(): async_read(), received_bytes = "
                              << received_bytes << " bytes." << std::endl;
                }
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "asio_connection::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t bytes_written)
            {
                if (!ec) {
                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter();

                    if ((uint32_t)bytes_written != packet_size_) {
                        std::cout << "asio_connection::do_write(): async_write(), bytes_written = "
                                  << bytes_written << " bytes." << std::endl;
                    }

                    do_read();
                }
                else {
                    // Write error log
                    std::cout << "asio_connection::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t received_bytes)
            {
                if ((uint32_t)received_bytes != packet_size_) {
                    std::cout << "asio_connection::do_read_some(): async_read(), received_bytes = "
                              << received_bytes << " bytes." << std::endl;
                }
                if (!ec) {
                    // A successful request, can be used to statistic qps
                    do_write_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_connection::do_read_some() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write_some()
    {
        //auto self(this->shared_from_this());
        socket_.async_write_some(boost::asio::buffer(data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t bytes_written)
            {
                if (!ec) {
                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter();

                    if ((uint32_t)bytes_written != packet_size_) {
                        std::cout << "asio_connection::do_write_some(): async_write(), bytes_written = "
                                  << bytes_written << " bytes." << std::endl;
                    }

                    do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_connection::do_write_some() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }
};

} // namespace asio_test

#undef USE_ATOMIC_REALTIME_UPDATE
#undef QUERY_COUNTER_INTERVAL
