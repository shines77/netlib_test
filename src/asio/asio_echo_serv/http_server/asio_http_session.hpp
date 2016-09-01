
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/smart_ptr.hpp>

#include "../common.h"

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

static const std::string g_response_html =
    "HTTP/1.1 200 OK\r\n"
    "Date: Fri, 31 Aug 2016 16:25:26 GMT\r\n"
    "Server: boost-asio\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: 12\r\n"
    "Connection: Keep-Alive\r\n\r\n"
    "Hello World!";

////////////////////////////////////////////////////////////////////////////////////
/*

                            < Http Ring Buffer >

 Bottom        Back         Parsed              Front                          Top
    |-----------|--------------|------------------|-----------------------------|
                ^              ^                  ^
*/
////////////////////////////////////////////////////////////////////////////////////

class http_ring_buffer {
private:
    std::unique_ptr<char> buffer_;
    std::size_t buffer_size_;
    char * top_;
    char * back_;
    char * parsed_;
    char * front_;

public:
    http_ring_buffer(std::size_t buffer_size)
        : buffer_size_(buffer_size), top_(nullptr), back_(nullptr),
          parsed_(nullptr), front_(nullptr) {
        init_ring_buffer(buffer_size);
    }
    ~http_ring_buffer() {}

private:
    void init_ring_buffer(std::size_t buffer_size) {
        char * newBuffer = new (std::nothrow) char [buffer_size * 2];
        if (newBuffer)
            ::memset(newBuffer, 0, buffer_size * 2 * sizeof(char));
        buffer_.reset(newBuffer);

        char * _bottom = buffer_.get();
        top_ = _bottom + buffer_size * 2;
        back_ = _bottom;
        parsed_ = _bottom;
        front_ = _bottom;
    }

public:
    std::size_t buffer_size() const { return buffer_size_; }
    std::size_t total_sizes() const { return buffer_size_ * 2; }
    std::size_t offset() const { return static_cast<std::size_t>(back_ - buffer_.get()); }
    std::size_t empty_size() const { return offset(); }
    std::size_t free_size() const { return static_cast<std::size_t>(top_ - front_); }
    std::size_t data_length() const { return static_cast<std::size_t>(front_ - back_); }

    std::size_t parse_pos() const { return static_cast<std::size_t>(parsed_ - back_); }

    char * data() const { return buffer_.get(); }

    char * bottom() const { return buffer_.get(); }
    char * top() const { return top_; }
    char * back() const { return back_; }
    char * parsed() const { return parsed_; }
    char * front() const { return front_; }

    void reset(std::size_t data_bytes, std::size_t parsed_pos) {
        char * _bottom = buffer_.get();
        back_ = _bottom;
        parsed_ = _bottom + parsed_pos;
        front_ = _bottom + data_bytes;
    }

    void rollback() {
        std::size_t empty_bytes = empty_size();
        std::size_t free_bytes = free_size();
        std::size_t data_bytes = data_length();
        std::size_t parsed_offset = parse_pos();

        if (data_bytes <= empty_bytes) {
            ::memcpy(bottom(), back(), data_bytes);
            reset(data_bytes, parsed_offset);
        }
        else {
            std::cout << "http_ring_buffer::rollback(): (data_bytes > empty_bytes) --> "
                      <<  data_bytes << " > " << empty_bytes << std::endl;
        }
    }

    bool read(std::size_t recv_size) {
        if (front_ + recv_size < top_) {
            front_ += recv_size;
            return true;
        }
        else {
            front_ = top_;
            return false;
        }
    }

    void parse_to(char * parsed) {
        back_ = parsed;
        parsed_ = parsed;
    }

    bool parse_add(std::size_t parse_size) {
        if (parsed_ + parse_size < top_) {
            parsed_ += parse_size;
            back_ = parsed_;
            return true;
        }
        else {
            parsed_ = top_;
            back_ = top_;
            return false;
        }
    }

    bool parse(char *& parsed) {
        char * cur = parsed_;
        while (cur <= (front_ - 4)) {
            if (cur[0] == '\r' && cur[1] == '\n'
                && cur[2] == '\r' && cur[3] == '\n') {
                parsed = (cur + 4);
                return true;
            }
            cur++;
        }
        parsed = (cur - 1);
        return false;
    }
};

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
    uint32_t    send_bytes_;
    uint32_t    recv_cnt_;
    uint32_t    sent_cnt_;

    uint32_t    recv_bytes_remain_;
    uint32_t    send_bytes_remain_;

    http_ring_buffer buffer_;

public:
    asio_http_session(boost::asio::io_service & io_service, uint32_t buffer_size,
                      uint32_t packet_size, uint32_t need_echo = mode_need_echo)
        : socket_(io_service), need_echo_(need_echo), buffer_size_(buffer_size), packet_size_(packet_size),
          query_count_(0), recv_bytes_(0), send_bytes_(0), recv_cnt_(0), sent_cnt_(0),
          recv_bytes_remain_(0), send_bytes_remain_(0), buffer_(buffer_size)
    {
        if (buffer_size_ > MAX_PACKET_SIZE)
            buffer_size_ = MAX_PACKET_SIZE;
        if (packet_size_ > MAX_PACKET_SIZE)
            packet_size_ = MAX_PACKET_SIZE;
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
            g_send_bytes.fetch_add(byte_sent);
        }
#else
        if (byte_sent > 0) {
            send_bytes_ += byte_sent;
            sent_cnt_++;
            if (sent_cnt_ >= MAX_UPDATE_CNT || send_bytes_ >= MAX_UPDATE_BYTES) {
                g_send_bytes.fetch_add(send_bytes_);
                send_bytes_ = 0;
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

    inline void do_query_counter_write_some(uint32_t send_bytes)
    {
        uint32_t delta_bytes = send_bytes_remain_ + send_bytes;
        if (delta_bytes >= packet_size_) {
            uint32_t delta_query_count = delta_bytes / packet_size_;
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_query_count > 0) {
                g_query_count.fetch_add(delta_query_count);
                send_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#else
            if (delta_query_count >= QUERY_COUNTER_INTERVAL) {
                g_query_count.fetch_add(delta_query_count);
                send_bytes_remain_ = delta_bytes - packet_size_ * delta_query_count;
                return;
            }
#endif
        }
        send_bytes_remain_ = delta_bytes;
    }

    void do_read()
    {
        boost::asio::async_read(socket_, boost::asio::buffer(buffer_.data(), packet_size_),
            [this](const boost::system::error_code & ec, std::size_t recv_bytes)
            {
                if ((uint32_t)recv_bytes != packet_size_) {
                    std::cout << "asio_http_session::do_read(): async_read(), recv_bytes = "
                              << recv_bytes << " bytes." << std::endl;
                }
                if (!ec) {
                    // Count the recieved bytes
                    do_recieve_counter((uint32_t)recv_bytes);

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
        boost::asio::async_write(socket_, boost::asio::buffer(buffer_.data(), packet_size_),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
                    // Count the sent bytes
                    do_send_counter((uint32_t)send_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter_write_some((uint32_t)send_bytes);

                    if ((uint32_t)send_bytes != packet_size_) {
                        std::cout << "asio_http_session::do_write(): async_write(), send_bytes = "
                                  << send_bytes << " bytes." << std::endl;
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
        static bool is_first_read = true;
        static int debug_output_cnt = 0;

        char * read_data = buffer_.front();
        std::size_t read_size = buffer_.free_size();

        socket_.async_read_some(boost::asio::buffer(read_data, read_size),
            [this](const boost::system::error_code & ec, std::size_t recv_bytes)
            {
                if (!ec) {
                    if (is_first_read) {
                        packet_size_ = (uint32_t)recv_bytes;
                        g_packet_size = packet_size_;
                        is_first_read = false;
                        std::cout << "recv_bytes = " << recv_bytes << std::endl;
                    }
                    else {
#if 0
                        if (debug_output_cnt < 20) {
                            std::cout << "recv_bytes = " << recv_bytes << std::endl;
                            debug_output_cnt++;
                        }
#endif
                    }
                    // Count the recieved bytes
                    do_recieve_counter((uint32_t)recv_bytes);

                    bool is_overflow = buffer_.read(recv_bytes);
                    if (!is_overflow) {
                        // Roll back the ring buffer
                        buffer_.rollback();

                        do_read_some();
                        return;
                    }
                    
                    char * scanned;
                    bool http_header_ok = buffer_.parse(scanned);
                    if (http_header_ok) {
                        buffer_.parse_to(scanned);

                        // A successful http request, can be used to statistic qps.
#if 1
                        do_write_http_response_some();
#else
                        do_write_http_response();
#endif
                    }
                    else {
                        do_read_some();
                    }
                }
                else {
                    // Write error log
#if 0
                    std::cout << "asio_http_session::do_read_some() - Error: (recv_bytes = " << recv_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
#endif
                    stop(true);
                }
            }
        );
    }

    void do_write_http_response() {
        static bool is_first_read = true;
        boost::asio::async_write(socket_, boost::asio::buffer(g_response_html.c_str(), g_response_html.size()),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
                    if (is_first_read) {
                        std::cout << "g_response_html.size() = " << g_response_html.size() << std::endl;
                        is_first_read = false;
                    }
                    // Count the sent bytes
                    do_send_counter((uint32_t)send_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter_write_some((uint32_t)send_bytes);

                    if ((uint32_t)send_bytes != g_response_html.size() && send_bytes != 0) {
                        std::cout << "asio_http_session::do_write_some(): async_write(), send_bytes = "
                                  << send_bytes << " bytes." << std::endl;
                    }

                    do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_write_some() - Error: (send_bytes = " << send_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );

        do_read_some();
    }

    void do_write_http_response_some() {
        static bool is_first_read = true;
        socket_.async_write_some(boost::asio::buffer(g_response_html.c_str(), g_response_html.size()),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
                    if (is_first_read) {
                        std::cout << "g_response_html.size() = " << g_response_html.size() << std::endl;
                        is_first_read = false;
                    }
                    // Count the sent bytes
                    do_send_counter((uint32_t)send_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_query_counter_write_some((uint32_t)send_bytes);

                    if ((uint32_t)send_bytes != g_response_html.size() && send_bytes != 0) {
                        std::cout << "asio_http_session::do_write_some(): async_write(), send_bytes = "
                                  << send_bytes << " bytes." << std::endl;
                    }

                    do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_write_some() - Error: (send_bytes = " << send_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                    stop(true);
                }
            }
        );
    }

    void do_write_some(int32_t total_send_bytes)
    {
        while (total_send_bytes > 0) {
            std::size_t buffer_size;
            if (total_send_bytes < PACKET_SIZE)
                buffer_size = total_send_bytes;
            else
                buffer_size = PACKET_SIZE;
#if 1
            // async write one time <= PACKET_SIZE
            boost::asio::async_write(socket_, boost::asio::buffer(buffer_.data(), buffer_size),
                [this, buffer_size](const boost::system::error_code & ec, std::size_t send_bytes)
                {
                    if (!ec) {
                        // Count the sent bytes
                        do_send_counter((uint32_t)send_bytes);

                        // If get a circle of ping-pong, we count the query one time.
                        do_query_counter_write_some((uint32_t)send_bytes);

                        if ((uint32_t)send_bytes != buffer_size) {
                            std::cout << "asio_http_session::do_write_some(): async_write(), send_bytes = "
                                      << send_bytes << " bytes." << std::endl;
                        }

                        do_read_some();
                    }
                    else {
                        // Write error log
                        std::cout << "asio_http_session::do_write_some() - Error: (send_bytes = " << send_bytes
                                  << ", code = " << ec.value() << ") "
                                  << ec.message().c_str() << std::endl;
                        stop(true);
                    }
                }
            );
#else
            // async write some one time <= PACKET_SIZE
            socket_.async_write_some(boost::asio::buffer(buffer_.data(), buffer_size),
                [this, buffer_size](const boost::system::error_code & ec, std::size_t send_bytes)
                {
                    if (!ec) {
                        // Count the sent bytes
                        do_send_counter((uint32_t)send_bytes);

                        // If get a circle of ping-pong, we count the query one time.
                        do_query_counter_write_some((uint32_t)send_bytes);

                        if ((uint32_t)send_bytes != buffer_size) {
                            std::cout << "asio_http_session::do_write_some(): async_write(), send_bytes = "
                                      << send_bytes << " bytes." << std::endl;
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
            total_send_bytes -= PACKET_SIZE;
        }
    }
};

} // namespace asio_test

#undef USE_ATOMIC_REALTIME_UPDATE
#undef QUERY_COUNTER_INTERVAL

#undef MAX_UPDATE_CNT
#undef MAX_UPDATE_BYTES
