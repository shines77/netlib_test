
#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <atomic>
#include <set>
#include <algorithm>
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

const std::string g_request_html_header =
        "GET /cookies HTTP/1.1\r\n"
        "Host: 127.0.0.1:8090\r\n"
        "Connection: keep-alive\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17\r\n"
        "Accept-Encoding: gzip,deflate,sdch\r\n"
        "Accept-Language: en-US,en;q=0.8\r\n"
        "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n"
        "Cookie: name=wookie\r\n"
        "\r\n";

const std::string g_response_html =
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
        if (newBuffer) {
            ::memset(newBuffer, 0, buffer_size * 2 * sizeof(char));
        }
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
        std::size_t offset = this->offset();
        std::size_t free_bytes = free_size();
        std::size_t data_bytes = data_length();
        std::size_t parsed_offset = parse_pos();

        if (offset >= 16) {
            ::memcpy((void *)bottom(), (void *)back(), data_bytes * sizeof(char));
        }
        else {
            ::memmove((void *)bottom(), (void *)back(), data_bytes * sizeof(char));
        }
        reset(data_bytes, parsed_offset);
    }

    bool read(std::size_t recv_size) {
        if (front_ + recv_size <= top_) {
            front_ += recv_size;
            return true;
        }
        else {
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

    bool parse(char * &parsed) {
        // Forward-tracking find method
        char * cur = parsed_;
        while (cur <= (front_ - 4)) {
            if (cur[0] == '\r' && cur[2] == '\r'
                && cur[1] == '\n' && cur[3] == '\n') {
                parsed = (cur + 4);
                return true;
            }
            cur++;
            assert(cur < top_);
        }
        parsed = cur;
        return false;
    }

    bool back_parse(char * &parsed) {
        // Back-tracking find method
        char * cur = front_ - 4;
        while (cur >= parsed_) {
            if (cur[0] == '\r') {
                if (cur[2] == '\r') {
                    if (cur[1] == '\n' && cur[3] == '\n') {
                        parsed = (cur + 4);
                        return true;
                    }
                    // TODO: If HTTP header request format is standard, this way is Ok.
                    cur -= 4;
                }
                else if (cur[1] == '\n') {
                    cur -= 2;
                }
                else {
                    cur -= 4;
                }
            }
            else if (cur[0] == '\n') {
                cur--;
            }
            else {
                cur -= 4;
            }
        }
        parsed = front_;
        return false;
    }
};

class connection_manager;

class asio_http_session : public std::enable_shared_from_this<asio_http_session>,
                          private boost::noncopyable {
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };

    /// Socket for the connection.
    ip::tcp::socket socket_;
    /// The manager for this connection.
    connection_manager * connection_manager_;

    bool        nodelay_;
    uint32_t    need_echo_;
    uint32_t    buffer_size_;
    uint32_t    packet_size_;
    uint64_t    recv_counter_;
    uint64_t    send_counter_;

    uint32_t    recv_bytes_;
    uint32_t    send_bytes_;
    uint32_t    recv_cnt_;
    uint32_t    send_cnt_;

    uint32_t    delta_recv_count_;
    uint32_t    delta_send_count_;
    uint32_t    recv_bytes_remain_;
    uint32_t    send_bytes_remain_;

    http_ring_buffer buffer_;

public:
    asio_http_session(boost::asio::io_service & io_service, connection_manager * manager, uint32_t buffer_size,
                      uint32_t packet_size, uint32_t need_echo = mode_need_echo)
        : socket_(io_service), connection_manager_(manager), nodelay_(false), need_echo_(need_echo),
          buffer_size_(buffer_size), packet_size_(packet_size),
          recv_counter_(0), send_counter_(0), recv_bytes_(0), send_bytes_(0), recv_cnt_(0), send_cnt_(0),
          delta_recv_count_(0), delta_send_count_(0), recv_bytes_remain_(0), send_bytes_remain_(0), buffer_(buffer_size)
    {
        nodelay_ = (g_nodelay != 0);
        if (buffer_size_ > MAX_PACKET_SIZE)
            buffer_size_ = MAX_PACKET_SIZE;
        if (packet_size_ > MAX_PACKET_SIZE)
            packet_size_ = MAX_PACKET_SIZE;
    }

    ~asio_http_session()
    {
    }

    void start()
    {
        set_socket_send_bufsize(MAX_PACKET_SIZE);
        set_socket_recv_bufsize(MAX_PACKET_SIZE);

        static const int kNetSendTimeout = 45 * 1000;    // Send timeout is 45 seconds.
        static const int kNetRecvTimeout = 45 * 1000;    // Recieve timeout is 45 seconds.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char *)&kNetSendTimeout, sizeof(kNetSendTimeout));
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char *)&kNetRecvTimeout, sizeof(kNetRecvTimeout));

        socket_.set_option(ip::tcp::no_delay(nodelay_));

        linger sLinger;
        sLinger.l_onoff = 1;    // Enable linger
        sLinger.l_linger = 5;   // After shutdown(), socket send/recv 5 second data yet.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_LINGER, (const char *)&sLinger, sizeof(sLinger));

        g_client_count++;

        if (g_first_time) {
            g_first_time = false;
            g_start_time = high_resolution_clock::now();
        }

        start_connection();

        do_read_some();
    }

    void stop()
    {
        if (socket_.native_handle()) {
            init_shutdown();

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT >= 0x0600)
            //socket_.cancel();
#endif
            socket_.close();

            if (g_client_count.load() != 0)
                g_client_count--;
        }
    }

    void start_connection();
    void stop_connection(const boost::system::error_code & ec);

    void init_shutdown()
    {
        // Initiate graceful connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }

    ip::tcp::socket & socket()
    {
        return socket_;
    }

    static boost::shared_ptr<asio_http_session> create_new(
        boost::asio::io_service & io_service, connection_manager * conn_manager, uint32_t buffer_size, uint32_t packet_size) {
        return boost::shared_ptr<asio_http_session>(new asio_http_session(io_service, conn_manager, buffer_size, packet_size, g_test_mode));
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

    inline void do_send_counter(uint32_t bytes_sent)
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        if (bytes_sent > 0) {
            g_send_bytes.fetch_add(bytes_sent);
        }
#else
        if (bytes_sent > 0) {
            send_bytes_ += bytes_sent;
            send_cnt_++;
            if (send_cnt_ >= MAX_UPDATE_CNT || send_bytes_ >= MAX_UPDATE_BYTES) {
                g_send_bytes.fetch_add(send_bytes_);
                send_bytes_ = 0;
                send_cnt_ = 0;
            }
        }
#endif
    }

    inline void do_recv_qps_counter()
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        g_query_count.fetch_add(1);
#else
        recv_counter_++;
        if (recv_counter_ >= QUERY_COUNTER_INTERVAL) {
            g_query_count.fetch_add(QUERY_COUNTER_INTERVAL);
            recv_counter_ = 0;
        }
#endif
    }

    inline void do_recv_qps_counter_read_some(uint32_t recv_bytes)
    {
        uint32_t delta_bytes = recv_bytes_remain_ + recv_bytes;
        if (delta_bytes >= g_request_html_header.size()) {
            uint64_t delta_send_count = delta_bytes / g_request_html_header.size();
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_send_count > 0) {
                g_query_count.fetch_add(delta_send_count);
                recv_bytes_remain_ = delta_bytes - g_request_html_header.size() * delta_send_count;
                return;
            }
#else
            if (delta_send_count >= QUERY_COUNTER_INTERVAL) {
                g_query_count.fetch_add(delta_send_count);
                recv_bytes_remain_ = delta_bytes - (uint32_t)(g_request_html_header.size() * delta_send_count);
                return;
            }
#endif
        }
        recv_bytes_remain_ = delta_bytes;
    }

    inline void do_send_counter_write_some(uint32_t send_bytes)
    {
        uint32_t delta_bytes = send_bytes_remain_ + send_bytes;
        static const uint32_t response_size = (uint32_t)g_response_html.size();
        if (delta_bytes >= response_size) {
            uint32_t delta_query_count = delta_bytes / response_size;
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
            if (delta_query_count > 0) {
                //g_query_count.fetch_add(delta_query_count);
                send_bytes_remain_ = delta_bytes - response_size * delta_query_count;
                return;
            }
#else
            if (delta_query_count >= QUERY_COUNTER_INTERVAL) {
                //g_query_count.fetch_add(delta_query_count);
                send_bytes_remain_ = delta_bytes - response_size * delta_query_count;
                return;
            }
#endif
        }
        send_bytes_remain_ = delta_bytes;
    }

    inline void do_send_counter_sync_write()
    {
#if defined(USE_ATOMIC_REALTIME_UPDATE) && (USE_ATOMIC_REALTIME_UPDATE > 0)
        g_query_count.fetch_add(1);
#else
        delta_send_count_++;
        if (delta_send_count_ >= QUERY_COUNTER_INTERVAL) {
            //g_query_count.fetch_add(delta_send_count_);
            delta_send_count_ = 0;
        }
#endif
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

                    stop_connection(ec);
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
                    do_send_counter_write_some((uint32_t)send_bytes);

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

                    stop_connection(ec);
                }
            }
        );
    }

    void do_read_some()
    {
        static bool is_first_read = true;
        static int debug_output_cnt = 0;

        if (buffer_.free_size() <= 1024) {
            // Roll back the ring buffer
            buffer_.rollback();
        }

        char * read_data = buffer_.front();
        std::size_t read_size = std::min(buffer_size_, (uint32_t)buffer_.free_size());
        assert(read_size > 0);

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

                    bool is_ok = buffer_.read(recv_bytes);
                    if (!is_ok) {
                        // Roll back the ring buffer
                        buffer_.rollback();

                        do_read_some();
                        return;
                    }
                    
                    char * scanned;
                    bool http_header_ok;
                    do {
                        http_header_ok = buffer_.parse(scanned);
                        if (http_header_ok) {
                            buffer_.parse_to(scanned);

                            do_recv_qps_counter();

                            // A successful http request, can be used to statistic qps.
                            if (!nodelay_) {
                                // nodelay = false;
#if 1
                                do_async_write_http_response();
#else
                                do_async_write_http_response_some();
#endif
                            }
                            else {
                                // nodelay = true;
                                do_sync_write_http_response();
                            }
                        }
                        else {
                            break;
                        }
                    } while (1);

                    do_read_some();
                }
                else {
                    // Write error log
#if 1
                    std::cout << "asio_http_session::do_read_some() - Error: (recv_bytes = " << recv_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
#endif
                    stop_connection(ec);
                }
            }
        );
    }

    void do_sync_write_http_response()
    {
        static bool is_first_read = true;
        boost::system::error_code ec;
        std::size_t send_bytes = boost::asio::write(socket_,
            boost::asio::buffer(g_response_html.c_str(), g_response_html.size()),
            ec);
        if (!ec) {
#if 0
            if (is_first_read) {
                std::cout << "g_response_html.size() = " << g_response_html.size() << std::endl;
                is_first_read = false;
            }
#endif
            // Count the sent bytes
            do_send_counter((uint32_t)send_bytes);

            // If get a circle of ping-pong, we count the query one time.
            do_send_counter_sync_write();

            if ((uint32_t)send_bytes != g_response_html.size() && send_bytes != 0) {
                std::cout << "asio_http_session::do_sync_write_http_response(): async_write(), send_bytes = "
                            << send_bytes << " bytes." << std::endl;
            }

            //do_read_some();
        }
        else {
            // Write error log
            std::cout << "asio_http_session::do_sync_write_http_response() - Error: (send_bytes = " << send_bytes
                        << ", code = " << ec.value() << ") "
                        << ec.message().c_str() << std::endl;

            stop_connection(ec);
        }
    }

    void do_async_write_http_response()
    {
        static bool is_first_read = true;
        boost::asio::async_write(socket_, boost::asio::buffer(g_response_html.c_str(), g_response_html.size()),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
#if 0
                    if (is_first_read) {
                        std::cout << "g_response_html.size() = " << g_response_html.size() << std::endl;
                        is_first_read = false;
                    }
#endif
                    // Count the sent bytes
                    do_send_counter((uint32_t)send_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_send_counter_sync_write();

                    if ((uint32_t)send_bytes != g_response_html.size() && send_bytes != 0) {
                        std::cout << "asio_http_session::do_async_write_http_response(): async_write(), send_bytes = "
                                  << send_bytes << " bytes." << std::endl;
                    }

                    //do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_async_write_http_response() - Error: (send_bytes = " << send_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;

                    stop_connection(ec);
                }
            }
        );
    }

    void do_async_write_http_response_some()
    {
        static bool is_first_read = true;
        socket_.async_write_some(boost::asio::buffer(g_response_html.c_str(), g_response_html.size()),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
#if 0
                    if (is_first_read) {
                        std::cout << "g_response_html.size() = " << g_response_html.size() << std::endl;
                        is_first_read = false;
                    }
#endif
                    // Count the sent bytes
                    do_send_counter((uint32_t)send_bytes);

                    // If get a circle of ping-pong, we count the query one time.
                    do_send_counter_sync_write();

                    if ((uint32_t)send_bytes != g_response_html.size() && send_bytes != 0) {
                        std::cout << "asio_http_session::do_async_write_http_response_some(): async_write(), send_bytes = "
                                  << send_bytes << " bytes." << std::endl;
                    }

                    //do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "asio_http_session::do_async_write_http_response_some() - Error: (send_bytes = " << send_bytes
                              << ", code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;

                    stop_connection(ec);
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
                        do_send_counter_write_some((uint32_t)send_bytes);

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

                        stop_connection(ec);
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
                        do_send_counter_write_some((uint32_t)send_bytes);

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

                        stop_connection(ec);
                    }
                }
            );
#endif
            total_send_bytes -= PACKET_SIZE;
        }
    }
};

typedef std::shared_ptr<asio_http_session> connection_ptr;

//
// See: https://www.boost.org/doc/libs/1_48_0/doc/html/boost_asio/example/http/server/connection.hpp
//

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager : private boost::noncopyable
{
private:
    /// The managed connections.
    std::set<connection_ptr> connections_;

public:
    /// Add the specified connection to the manager and start it.
    void connection_manager::start(connection_ptr connection)
    {
        connections_.insert(connection);
        //connection->start();
    }

    /// Stop the specified connection.
    void connection_manager::stop(connection_ptr connection)
    {
        connections_.erase(connection);
        connection->stop();
    }

    /// Stop all connections.
    void connection_manager::stop_all()
    {
        std::for_each(connections_.begin(), connections_.end(), std::bind(&asio_http_session::stop, std::placeholders::_1));
        connections_.clear();
    }
};

void asio_http_session::start_connection()
{
    //if (connection_manager_)
    //    connection_manager_->start(asio_http_session::shared_from_this());
}

void asio_http_session::stop_connection(const boost::system::error_code & ec)
{
    if (ec != boost::asio::error::operation_aborted)
    {
        //if (connection_manager_)
        //    connection_manager_->stop(asio_http_session::shared_from_this());
        stop();
    }
}

} // namespace asio_test

#undef USE_ATOMIC_REALTIME_UPDATE
#undef QUERY_COUNTER_INTERVAL

#undef MAX_UPDATE_CNT
#undef MAX_UPDATE_BYTES
