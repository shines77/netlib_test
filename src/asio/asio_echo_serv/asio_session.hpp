
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "common.h"

using namespace boost::system;

#define MAX_PACKET_SIZE	            65536

// Whether use atomic update realtime?
#define USE_ATOMIC_REALTIME_UPDATE  1

#define QUERY_COUNTER_INTERVAL      100

#define MAX_UPDATE_CNT              3
#define MAX_UPDATE_BYTES            32768

using namespace boost::asio;

namespace asio_test {

class asio_session : public std::enable_shared_from_this<asio_session>,
                     private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    ip::tcp::socket socket_;
    uint32_t    buffer_size_;
    uint32_t    packet_size_;
    uint64_t    query_count_;

    uint32_t    recieved_bytes_;
    uint32_t    sent_bytes_;

    uint32_t    recieved_cnt_;
    uint32_t    sent_cnt_;

    uint32_t    sent_bytes_remain_;

    char data_[PACKET_SIZE];

public:
    asio_session(boost::asio::io_service & io_service, uint32_t buffer_size, uint32_t packet_size)
        : socket_(io_service), buffer_size_(buffer_size_), packet_size_(packet_size),
          query_count_(0),
          recieved_bytes_(0), sent_bytes_(0), recieved_cnt_(0), sent_cnt_(0),
          sent_bytes_remain_(0)
    {
        if (buffer_size_ > MAX_PACKET_SIZE)
            buffer_size_ = MAX_PACKET_SIZE;
        if (packet_size_ > MAX_PACKET_SIZE)
            packet_size_ = MAX_PACKET_SIZE;
        ::memset(data_, 0, sizeof(data_));
    }

    ~asio_session()
    {
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT >= 0x0600)
        socket_.cancel();
#endif
        socket_.shutdown(socket_base::shutdown_both);
        socket_.close();
    }

    void start()
    {
        g_client_count++;
        set_socket_recv_bufsize(MAX_PACKET_SIZE);
        do_read_some();
    }

    void stop(bool delete_self = false)
    {
        g_client_count--;
        if (delete_self)
            delete this;
    }

    ip::tcp::socket & socket()
    {
        return socket_;
    }

    static boost::shared_ptr<asio_session> create_new(
        boost::asio::io_service & io_service, uint32_t buffer_size, uint32_t packet_size) {
        return boost::shared_ptr<asio_session>(new asio_session(io_service, buffer_size, packet_size));
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

    inline void do_recieve_counter(uint32_t byte_recieved)
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        if (byte_recieved > 0) {
            g_recieved_bytes.fetch_add(byte_recieved);
        }
#else
        if (byte_recieved > 0) {
            recieved_bytes_ += byte_recieved;
            recieved_cnt_++;
            if (recieved_cnt_ >= MAX_UPDATE_CNT || recieved_bytes_ >= MAX_UPDATE_BYTES) {
                g_recieved_bytes.fetch_add(recieved_bytes_);
                recieved_bytes_ = 0;
                recieved_cnt_ = 0;
            }
        }
#endif
    }

    inline void do_send_counter(uint32_t byte_sent)
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        if (byte_sent > 0) {
            g_sent_bytes.fetch_add(byte_sent);
        }
#else
        if (byte_sent > 0) {
            sent_bytes_ += byte_sent;
            sent_cnt_++;
            if (sent_cnt_ >= MAX_UPDATE_CNT || sent_bytes_ >= MAX_UPDATE_BYTES) {
                g_sent_bytes.fetch_add(sent_bytes_);
                sent_bytes_ = 0;
                sent_cnt_ = 0;
            }
        }
#endif
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

    inline void do_query_counter_some(uint32_t sent_bytes)
    {
        uint32_t delta_bytes = sent_bytes_remain_ + sent_bytes_;
        if (delta_bytes >= packet_size_) {
            uint32_t delta_query_count = delta_bytes / packet_size_;
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_query_count > 0) {
                g_query_count.fetch_add(delta_query_count);
                sent_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#else
            query_count_ += delta_query_count;
            if (query_count_ >= QUERY_COUNTER_INTERVAL) {
                g_query_count.fetch_add(QUERY_COUNTER_INTERVAL);
                query_count_ = 0;
                sent_bytes_remain_ = delta_bytes - packet_size_ * QUERY_COUNTER_INTERVAL;
                return;
            }
#endif
        }
        sent_bytes_remain_ = delta_bytes;
    }

    void do_read()
    {
        //auto self(this->shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t received_bytes)
            {
                if ((uint32_t)received_bytes != packet_size_) {
                    std::cout << "asio_session::do_read(): async_read(), received_bytes = "
                              << received_bytes << " bytes." << std::endl;
                }
                if (!ec) {
                    // Count the recieved bytes
                    do_recieve_counter((uint32_t)received_bytes);

                    // A successful request, can be used to statistic qps
                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_read() - Error: (code = " << ec.value() << ") "
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
            [this](boost::system::error_code ec, std::size_t sent_bytes)
            {
                if (!ec) {
                    // Count the sent bytes
                    do_send_counter((uint32_t)sent_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter();

                    if ((uint32_t)sent_bytes != packet_size_) {
                        std::cout << "asio_session::do_write(): async_write(), sent_bytes = "
                                  << sent_bytes << " bytes." << std::endl;
                    }

                    do_read();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(data_, buffer_size_),
            [this](boost::system::error_code ec, std::size_t received_bytes)
            {
                if ((uint32_t)received_bytes != buffer_size_) {
                    std::cout << "asio_session::do_read_some(): async_read(), received_bytes = "
                              << received_bytes << " bytes." << std::endl;
                }
                if (!ec) {
                    // Count the recieved bytes
                    do_recieve_counter((uint32_t)received_bytes);

                    // A successful request, can be used to statistic qps
                    do_write_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write_some()
    {
        //auto self(this->shared_from_this());
        socket_.async_write_some(boost::asio::buffer(data_, buffer_size_),
            [this](boost::system::error_code ec, std::size_t sent_bytes)
            {
                if (!ec) {
                    // Count the sent bytes
                    do_send_counter((uint32_t)sent_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter_some((uint32_t)sent_bytes);

                    if ((uint32_t)sent_bytes != buffer_size_) {
                        std::cout << "asio_session::do_write_some(): async_write(), sent_bytes = "
                                  << sent_bytes << " bytes." << std::endl;
                    }

                    do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_session::do_write() - Error: (code = " << ec.value() << ") "
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

#undef MAX_UPDATE_CNT
#undef MAX_UPDATE_BYTES
