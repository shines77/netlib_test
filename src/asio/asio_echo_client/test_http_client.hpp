
#pragma once

#include <iostream>
#include <iomanip>      // For std::setw()
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

#include "common.h"

using namespace boost::asio;
using namespace std::chrono;

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

class test_http_client
{
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };
    enum { kSendRepeatTimes = 20 };
    
    boost::asio::io_service & io_service_;
    ip::tcp::socket socket_;
    uint32_t mode_;
    uint32_t buffer_size_;
    uint32_t packet_size_;
    size_t html_header_size_;
    size_t html_response_size_;

    uint64_t last_query_count_;
    uint64_t total_query_count_;
    double last_total_latency_;
    double total_latency_;

    duration<double> elapsed_time_;
    time_point<high_resolution_clock> send_time_;
    time_point<high_resolution_clock> recieve_time_;
    time_point<high_resolution_clock> last_time_;

    uint32_t send_bytes_;
    uint32_t recieved_bytes_;

    uint32_t sent_cnt_;

    char recv_data_[PACKET_SIZE];
    char send_data_[PACKET_SIZE];

public:
    test_http_client(boost::asio::io_service & io_service,
        ip::tcp::resolver::iterator endpoint_iterator, uint32_t mode, uint32_t buffer_size, uint32_t packet_size)
        : io_service_(io_service),
          socket_(io_service), mode_(mode), buffer_size_(buffer_size), packet_size_(packet_size), html_header_size_(0),
          last_query_count_(0), total_query_count_(0), last_total_latency_(0.0), total_latency_(0.0),
          send_bytes_(0), recieved_bytes_(0), sent_cnt_(0)
    {
        html_header_size_ = g_request_html_header.size();
        html_response_size_ = g_response_html.size();

        ::memset(recv_data_, 0, sizeof(recv_data_));
        ::memset(send_data_, 0, sizeof(send_data_));
        ::memcpy((void *)&send_data_[0], (void *)g_request_html_header.c_str(), html_header_size_);

        last_time_ = high_resolution_clock::now();

        do_connect(endpoint_iterator);
    }

    ~test_http_client()
    {
        time_point<high_resolution_clock> startime = high_resolution_clock::now();

        time_point<high_resolution_clock> endtime = high_resolution_clock::now();
        duration<double> elapsed_time = duration_cast< duration<double> >(endtime - startime);
    }

private:
    int get_socket_send_bufsize() const
    {
        boost::asio::socket_base::send_buffer_size send_bufsize_option;
        socket_.get_option(send_bufsize_option);

        //std::cout << "send_buffer_size: " << send_bufsize_option.value() << " bytes" << std::endl;
        return send_bufsize_option.value();
    }

    void set_socket_send_bufsize(int buffer_size)
    {
        boost::asio::socket_base::receive_buffer_size send_bufsize_option(buffer_size);
        socket_.set_option(send_bufsize_option);

        //std::cout << "set_socket_send_buffer_size(): " << buffer_size << " bytes" << std::endl;
    }

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

    void display_counters()
    {
        elapsed_time_ = duration_cast< duration<double> >(recieve_time_ - send_time_);
        last_query_count_++;
        total_query_count_++;
        last_total_latency_ += elapsed_time_.count();
        total_latency_ += elapsed_time_.count();
        time_point<high_resolution_clock> now_time = high_resolution_clock::now();
        duration<double> interval_time = duration_cast< duration<double> >(now_time - last_time_);
        double elapsed_time = interval_time.count();
        double avg_latency, avg_total_latency;
        if (elapsed_time > 1.0) {
            std::cout << "packet_size           = " << std::left << std::setw(8)
                      << packet_size_ << " B,  latency total     = "
                      << last_total_latency_ << " ms" <<  std::endl;
            // average latency (one second interval)
            if (last_query_count_ != 0)
                avg_latency = (last_total_latency_ * 1000.0) / (double)last_query_count_;
            else
                avg_latency = 0.0;
            std::cout << "average latency       = " << std::left << std::setw(8)
                      << std::setiosflags(std::ios::fixed) << std::setprecision(6)
                      << avg_latency << " ms, query count       = " << last_query_count_ << std::endl;

            // average latency total
            if (total_query_count_ != 0)
                avg_total_latency = (total_latency_ * 1000.0) / (double)total_query_count_;
            else
                avg_total_latency = 0.0;
            std::cout << "average latency total = " << std::left << std::setw(8)
                      << std::setiosflags(std::ios::fixed) << std::setprecision(6)
                      << avg_total_latency << " ms, query count total = " << total_query_count_ << std::endl;

            std::cout << "send bytes: " << send_bytes_ << ", recieved bytes: " << recieved_bytes_
                      << ", send pkgs: " << (send_bytes_ / packet_size_) << ", recieved pkgs: " << (recieved_bytes_ / packet_size_)
                      << std::endl;
            std::cout << std::endl;

            // Reset the counters
            last_time_ = high_resolution_clock::now();
            last_query_count_ = 0;
            last_total_latency_ = 0.0;
        }
    }

    void start()
    {
        set_socket_send_bufsize(MAX_PACKET_SIZE);
        set_socket_recv_bufsize(MAX_PACKET_SIZE);

        static const int nNetSendTimeout = 45 * 1000;    // Send timeout is 60 second.
        static const int nNetRecvTimeout = 45 * 1000;    // Recieve timeout is 60 second.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char *)&nNetSendTimeout, sizeof(nNetSendTimeout));
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char *)&nNetRecvTimeout, sizeof(nNetRecvTimeout));

        linger sLinger;
        sLinger.l_onoff = 1;    // Enable linger
        sLinger.l_linger = 5;   // After shutdown(), socket send/recv 5 second data yet.
        ::setsockopt(socket_.native_handle(), SOL_SOCKET, SO_LINGER, (const char *)&sLinger, sizeof(sLinger));

        if (mode_ == test_method_throughput) {
            //do_sync_write_only();
            do_async_write_only();
        }
        else {
            //do_write();
            do_sync_write(kSendRepeatTimes);
        }
    }

    void stop(bool delete_self = false)
    {
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT >= 0x0600)
        socket_.cancel();
#endif

        //socket_.shutdown(socket_base::shutdown_both);
        if (socket_.is_open()) {
            socket_.close();
        }

        if (delete_self)
            delete this;
    }

    void do_connect(ip::tcp::resolver::iterator endpoint_iterator)
    {
        boost::asio::async_connect(socket_, endpoint_iterator,
            [this](const boost::system::error_code & ec, ip::tcp::resolver::iterator)
            {
                if (!ec)
                {
                    start();
                }
            });
    }

    void do_read()
    {
        boost::asio::async_read(socket_,
            boost::asio::buffer(recv_data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t recieved_bytes)
            {
                if ((uint32_t)recieved_bytes != html_response_size_) {
                    std::cout << "test_http_client::do_read(): async_read(), recieved_bytes = "
                              << recieved_bytes << " bytes." << std::endl;
                }
                if (!ec)
                {
                    // Have recieved the response message
                    recieve_time_ = high_resolution_clock::now();
                    if (recieved_bytes > 0)
                        recieved_bytes_ += (uint32_t)recieved_bytes;

                    display_counters();

                    do_write();
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_read() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(recv_data_, buffer_size_),
            [this](const boost::system::error_code & ec, std::size_t recieved_bytes)
            {
                if ((uint32_t)recieved_bytes != html_response_size_) {
                    std::cout << "test_http_client::do_read_some(): async_read(), recieved_bytes = "
                              << recieved_bytes << " bytes." << std::endl;
                }
                if (!ec)
                {
                    // Have recieved the response message
                    recieve_time_ = high_resolution_clock::now();
                    if (recieved_bytes > 0)
                        recieved_bytes_ += (uint32_t)recieved_bytes;

                    display_counters();

                    do_write_some();
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_read_some() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_sync_read_some()
    {
        socket_.async_read_some(boost::asio::buffer(recv_data_, buffer_size_),
            [this](const boost::system::error_code & ec, std::size_t recieved_bytes)
            {
                /*
                if ((uint32_t)recieved_bytes != html_response_size_) {
                    std::cout << "test_http_client::do_sync_read_some(): async_read(), recieved_bytes = "
                              << recieved_bytes << " bytes." << std::endl;
                }
                //*/

                if (!ec)
                {
                    // Have recieved the response message
                    recieve_time_ = high_resolution_clock::now();
                    if (recieved_bytes > 0)
                        recieved_bytes_ += (uint32_t)recieved_bytes;

                    display_counters();

                    do_sync_write(kSendRepeatTimes);
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_sync_read_some() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_sync_read_only()
    {
        socket_.async_read_some(boost::asio::buffer(recv_data_, buffer_size_),
            [this](const boost::system::error_code & ec, std::size_t recieved_bytes)
            {
                if ((uint32_t)recieved_bytes != html_response_size_) {
                    std::cout << "test_http_client::do_sync_read_only(): async_read_some(), recieved_bytes = "
                              << recieved_bytes << " bytes." << std::endl;
                }

                if (!ec)
                {
                    // Have recieved the response message
                    recieve_time_ = high_resolution_clock::now();
                    if (recieved_bytes > 0)
                        recieved_bytes_ += (uint32_t)recieved_bytes;

                    display_counters();

                    do_sync_write_only();
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_sync_read_only() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_write()
    {
        // Prepare to send the request message
        send_time_ = high_resolution_clock::now();

        boost::asio::async_write(socket_,
            boost::asio::buffer(send_data_, html_header_size_),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if ((uint32_t)send_bytes != html_header_size_) {
                    std::cout << "test_http_client::do_write(): async_write(), send_bytes = "
                              << send_bytes << " bytes." << std::endl;
                }

                if (!ec)
                {
                    send_bytes_ += (uint32_t)send_bytes;
                    do_read();
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_write_some()
    {
        // Prepare to send the request message
        send_time_ = high_resolution_clock::now();

        boost::asio::async_write(socket_,
            boost::asio::buffer(send_data_, html_header_size_),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if ((uint32_t)send_bytes != html_header_size_) {
                    std::cout << "test_http_client::do_write(): async_write(), send_bytes = "
                              << send_bytes << " bytes." << std::endl;
                }

                if (!ec)
                {
                    send_bytes_ += (uint32_t)send_bytes;
                    do_read_some();
                }
                else {
                    // Write error log
                    std::cout << "test_http_client::do_write() - Error: (code = " << ec.value() << ") "
                              << ec.message().c_str() << std::endl;
                }
            });
    }

    void do_sync_write(int repeat)
    {
        // Prepare to send the request message
        send_time_ = high_resolution_clock::now();

        for (int i = 0; i < repeat; ++i) {
            boost::system::error_code ec;
            std::size_t send_bytes = socket_.send(boost::asio::buffer(send_data_, html_header_size_), 0, ec);
            if (!ec) {
                send_bytes_ += (uint32_t)send_bytes;
            }
            else {
                std::cout << "test_http_client::do_sync_write() - Error: (code = " << ec.value() << ") "
                          << ec.message().c_str() << std::endl;
            }
        }

        do_sync_read_some();
    }

    void do_async_write_only()
    {
        // Prepare to send the request message
        last_time_ = high_resolution_clock::now();

        sent_cnt_ = 0;
        send_bytes_ = 0;

        do_sync_write_only();
    }

    void do_sync_write_only()
    {
#if 0
        // Prepare to send the request message
        time_point<high_resolution_clock> last_time = high_resolution_clock::now();

        static unsigned int sent_cnt = 0;
        for (;;) {
            boost::system::error_code ec;
            std::size_t send_bytes = socket_.send(boost::asio::buffer(send_data_, html_header_size_), 0, ec);
            if (!ec) {
                if (send_bytes > 0) {
                    sent_cnt++;
                    send_bytes_ += (uint32_t)send_bytes;
                    if ((sent_cnt & 0x7FFF) == 0x7FFF) {
                        time_point<high_resolution_clock> now_time = high_resolution_clock::now();
                        duration<double> interval_time = duration_cast< duration<double> >(now_time - last_time);
                        double elapsed_time = interval_time.count();
                        std::cout << "sent_cnt = " << sent_cnt_ << ", send_bytes = " << send_bytes_ << ", BandWidth = "
                                  << std::left << std::setw(5)
                                  << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                                  << (send_bytes_ / (1000.0 * 1000.0) / (double)elapsed_time) << " MB/s"
                                  << std::endl;
                        last_time = high_resolution_clock::now();
                        send_bytes_ = 0;
                    }
                }
            }
            else {
                std::cout << "test_http_client::do_sync_write_only() - Error: (code = " << ec.value() << ") "
                          << ec.message().c_str() << std::endl;
            }
        }
#else
        boost::asio::async_write(socket_, boost::asio::buffer(send_data_, html_header_size_),
            [this](const boost::system::error_code & ec, std::size_t send_bytes)
            {
                if (!ec) {
                    if (send_bytes > 0) {
                        sent_cnt_++;
                        send_bytes_ += (uint32_t)send_bytes;
                        time_point<high_resolution_clock> now_time = high_resolution_clock::now();
                        duration<double> interval_time = duration_cast< duration<double> >(now_time - last_time_);
                        double elapsed_time = interval_time.count();
                        if (elapsed_time > 1.0 ) {                           
                            std::cout << "sent_cnt = " << sent_cnt_ << ", send_bytes = " << send_bytes_ << ", BandWidth = "
                                      << std::left << std::setw(5)
                                      << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                                      << (send_bytes_ / (1000.0 * 1000.0) / (double)elapsed_time) << " MB/s"
                                      << std::endl;
                            last_time_ = high_resolution_clock::now();
                            sent_cnt_ = 0;
                            send_bytes_ = 0;
                        }
                    }

                    do_sync_write_only();
                }
                else {
                    std::cout << "test_http_client::do_sync_write_only() - Error: (code = " << ec.value() << ") "
                                << ec.message().c_str() << std::endl;
                }
        });
#endif
    }
};

} // namespace asio_test
