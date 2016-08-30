
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/smart_ptr.hpp>

#include "common.h"

using namespace boost::system;

#define MIN_PACKET_SIZE             64
#define MAX_PACKET_SIZE	            (64 * 1024)

// Whether use atomic update realtime?
#define USE_ATOMIC_REALTIME_UPDATE  0

#define QUERY_COUNTER_INTERVAL      99

#define MAX_UPDATE_CNT              5
#define MAX_UPDATE_BYTES            32768

using namespace boost::asio;

namespace asio_test {

class asio_http_session : public boost::enable_shared_from_this<asio_http_session>,
                          private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    ip::tcp::socket socket_;
    uint32_t    need_echo_;
    uint32_t    buffer_size_;
    uint32_t    packet_size_;
    uint64_t    query_count_;

    uint32_t    recv_bytes_;
    uint32_t    sent_bytes_;
    uint32_t    recv_cnt_;
    uint32_t    sent_cnt_;

    uint32_t    recv_bytes_remain_;
    uint32_t    sent_bytes_remain_;

    std::unique_ptr<char> data_;

public:
    asio_http_session(boost::asio::io_service & io_service, uint32_t buffer_size,
                      uint32_t packet_size, uint32_t need_echo = mode_need_echo)
        : socket_(io_service), need_echo_(need_echo), buffer_size_(buffer_size), packet_size_(packet_size),
          query_count_(0), recv_bytes_(0), sent_bytes_(0), recv_cnt_(0), sent_cnt_(0),
          recv_bytes_remain_(0), sent_bytes_remain_(0)
    {
        if (buffer_size_ > MAX_PACKET_SIZE)
            buffer_size_ = MAX_PACKET_SIZE;
        if (packet_size_ > MAX_PACKET_SIZE)
            packet_size_ = MAX_PACKET_SIZE;

        char * newData = new (std::nothrow) char [buffer_size];
        if (newData)
            ::memset(newData, 0, buffer_size * sizeof(char));
        data_.reset(newData);
    }

    ~asio_http_session()
    {
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT >= 0x0600)
        socket_.cancel();
#endif
        //socket_.shutdown(socket_base::shutdown_both);
        socket_.close();
    }

    void start()
    {
        g_client_count++;
        set_socket_send_bufsize(MAX_PACKET_SIZE);
        set_socket_recv_bufsize(MAX_PACKET_SIZE);

        static const int kNetSendTimeout = 45 * 1000;    // Send timeout is 45 seconds.
        static const int kNetRecvTimeout = 45 * 1000;    // Recieve timeout is 45 seconds.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char *)&kNetSendTimeout, sizeof(kNetSendTimeout));
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char *)&kNetRecvTimeout, sizeof(kNetRecvTimeout));

        linger sLinger;
        sLinger.l_onoff = 1;    // Enable linger
        sLinger.l_linger = 5;   // After shutdown(), socket send/recv 5 second data yet.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_LINGER, (const char *)&sLinger, sizeof(sLinger));

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

    static boost::shared_ptr<asio_http_session> create_new(
        boost::asio::io_service & io_service, uint32_t buffer_size, uint32_t packet_size) {
        return boost::shared_ptr<asio_http_session>(new asio_http_session(io_service, buffer_size, packet_size, g_test_mode));
    }

private:
    int get_socket_send_bufsize() const
    {
        boost::asio::socket_base::send_buffer_size send_bufsize_option;
        socket_.get_option(send_bufsize_option);
        return send_bufsize_option.value();
    }

    void set_socket_send_bufsize(int buffer_size)
    {
        boost::asio::socket_base::receive_buffer_size send_bufsize_option(buffer_size);
        socket_.set_option(send_bufsize_option);
    }

    int get_socket_recv_bufsize() const
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option;
        socket_.get_option(recv_bufsize_option);
        return recv_bufsize_option.value();
    }

    void set_socket_recv_bufsize(int buffer_size)
    {
        boost::asio::socket_base::receive_buffer_size recv_bufsize_option(buffer_size);
        socket_.set_option(recv_bufsize_option);
    }

    inline void do_recieve_counter(uint32_t bytes_recieved)
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        if (bytes_recieved > 0) {
            g_recv_bytes.fetch_add(bytes_recieved);
        }
#else
        if (bytes_recieved > 0) {
            recv_bytes_ += bytes_recieved;
            recv_cnt_++;
            if (recv_cnt_ >= MAX_UPDATE_CNT || recv_bytes_ >= MAX_UPDATE_BYTES) {
                g_recv_bytes.fetch_add(recv_bytes_);
                recv_bytes_ = 0;
                recv_cnt_ = 0;
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

    inline void do_query_counter_read_some(uint32_t recv_bytes)
    {
        uint32_t delta_bytes = recv_bytes_remain_ + recv_bytes;
        if (delta_bytes >= packet_size_) {
            uint32_t delta_query_count = delta_bytes / packet_size_;
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_query_count > 0) {
                g_query_count.fetch_add(delta_query_count);
                recv_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#else
            if (delta_query_count >= QUERY_COUNTER_INTERVAL) {
                g_query_count.fetch_add(delta_query_count);
                recv_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#endif
        }
        recv_bytes_remain_ = delta_bytes;
    }

    inline void do_query_counter_write_some(uint32_t sent_bytes)
    {
        uint32_t delta_bytes = sent_bytes_remain_ + sent_bytes;
        if (delta_bytes >= packet_size_) {
            uint32_t delta_query_count = delta_bytes / packet_size_;
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_query_count > 0) {
                g_query_count.fetch_add(delta_query_count);
                sent_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#else
            if (delta_query_count >= QUERY_COUNTER_INTERVAL) {
                g_query_count.fetch_add(delta_query_count);
                sent_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#endif
        }
        sent_bytes_remain_ = delta_bytes;
    }

    void do_read()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(data_.get(), packet_size_),
            [this](const boost::system::error_code & ec, std::size_t received_bytes)
            {
                if ((uint32_t)received_bytes != packet_size_) {
                    std::cout << "asio_http_session::do_read(): async_read(), received_bytes = "
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
                    std::cout << "asio_http_session::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write()
    {
        boost::asio::async_write(socket_, boost::asio::buffer(data_.get(), packet_size_),
            [this](const boost::system::error_code & ec, std::size_t sent_bytes)
            {
                if (!ec) {
                    // Count the sent bytes
                    do_send_counter((uint32_t)sent_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter_write_some((uint32_t)sent_bytes);

                    if ((uint32_t)sent_bytes != packet_size_) {
                        std::cout << "asio_http_session::do_write(): async_write(), sent_bytes = "
                                  << sent_bytes << " bytes." << std::endl;
                    }

                    do_read();
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(data_.get(), buffer_size_),
            [this](const boost::system::error_code & ec, std::size_t received_bytes)
            {
                if (!ec) {
                    // Count the recieved bytes
                    do_recieve_counter((uint32_t)received_bytes);

                    if (need_echo_ == mode_no_echo) {
                        // Counter the recieved qps
                        do_query_counter_read_some((int32_t)received_bytes);

                        // Needn't respond the request and read data again.
                        do_read_some();
                    }
                    else {
                        // A successful request, can be used to statistic qps.
                        do_write_some((int32_t)received_bytes);
                    }
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_read_some() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write_some(int32_t total_sent_bytes)
    {
        //auto self(this->shared_from_this());
        while (total_sent_bytes > 0) {
            std::size_t buffer_size;
            if (total_sent_bytes < PACKET_SIZE)
                buffer_size = total_sent_bytes;
            else
                buffer_size = PACKET_SIZE;
#if 1
            // async write one time <= PACKET_SIZE
            boost::asio::async_write(socket_, boost::asio::buffer(data_.get(), buffer_size),
                [this, buffer_size](const boost::system::error_code & ec, std::size_t sent_bytes)
                {
                    if (!ec) {
                        // Count the sent bytes
                        do_send_counter((uint32_t)sent_bytes);

                        // If get a circle of ping-pong, we count the query one time.
                        do_query_counter_write_some((uint32_t)sent_bytes);

                        if ((uint32_t)sent_bytes != buffer_size) {
                            std::cout << "asio_http_session::do_write_some(): async_write(), sent_bytes = "
                                      << sent_bytes << " bytes." << std::endl;
                        }

                        do_read_some();
                    }
                    else {
                        // Write error log
                        std::cout << "asio_http_session::do_write_some() - Error: (code = " << ec.value() << ") "
                                  << ec.message().c_str() << std::endl;
                        stop(true);
                    }
                }
            );
#else
            // async write some one time <= PACKET_SIZE
            socket_.async_write_some(boost::asio::buffer(data_.get(), buffer_size),
                [this, buffer_size](const boost::system::error_code & ec, std::size_t sent_bytes)
                {
                    if (!ec) {
                        // Count the sent bytes
                        do_send_counter((uint32_t)sent_bytes);

                        // If get a circle of ping-pong, we count the query one time.
                        do_query_counter_write_some((uint32_t)sent_bytes);

                        if ((uint32_t)sent_bytes != buffer_size) {
                            std::cout << "asio_http_session::do_write_some(): async_write(), sent_bytes = "
                                      << sent_bytes << " bytes." << std::endl;
                        }

                        do_read_some();
                    }
                    else {
                        // Write error log
                        std::cout << "asio_http_session::do_write() - Error: (code = " << ec.value() << ") "
                                  << ec.message().c_str() << std::endl;
                        stop(true);
                    }
                }
            );
#endif
            total_sent_bytes -= PACKET_SIZE;
        }
    }
};

} // namespace asio_test

#undef USE_ATOMIC_REALTIME_UPDATE
#undef QUERY_COUNTER_INTERVAL

#undef MAX_UPDATE_CNT
#undef MAX_UPDATE_BYTES
