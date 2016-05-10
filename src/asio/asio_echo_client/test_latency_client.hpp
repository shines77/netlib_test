
#pragma once

#include <iostream>
#include <iomanip>      // For std::setw()
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

#include "common.h"

#define MAX_PACKET_SIZE	65536

using namespace boost::asio;
using namespace std::chrono;

namespace asio_test {

class test_latency_client
{
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };
    
    boost::asio::io_service & io_service_;
    ip::tcp::socket socket_;
    std::uint32_t packet_size_;

    uint64_t last_query_count_;
    uint64_t total_query_count_;
    double last_total_latency_;
    double total_latency_;

    duration<double> elapsed_time_;
    time_point<high_resolution_clock> send_time_;
    time_point<high_resolution_clock> recieve_time_;
    time_point<high_resolution_clock> last_time_;

    char data_[PACKET_SIZE];

public:
    test_latency_client(boost::asio::io_service & io_service,
        ip::tcp::resolver::iterator endpoint_iterator, std::uint32_t packet_size)
        : io_service_(io_service),
          socket_(io_service), packet_size_(packet_size),
          last_query_count_(0), total_query_count_(0), last_total_latency_(0.0), total_latency_(0.0)
    {
        ::memset(data_, 'h', sizeof(data_));
        last_time_ = high_resolution_clock::now();
        do_connect(endpoint_iterator);
    }

    ~test_latency_client()
    {
        time_point<high_resolution_clock> startime = high_resolution_clock::now();

        time_point<high_resolution_clock> endtime = high_resolution_clock::now();
        duration<double> elapsed_time = duration_cast< duration<double> >(endtime - startime);
    }

private:
    void display_counters()
    {
        elapsed_time_ = duration_cast< duration<double> >(recieve_time_ - send_time_);
        last_query_count_++;
        total_query_count_++;
        last_total_latency_ += elapsed_time_.count();
        total_latency_ += elapsed_time_.count();
        time_point<high_resolution_clock> now_time = high_resolution_clock::now();
        duration<double> interval_time = duration_cast< duration<double> >(now_time - last_time_);
        double avg_latency, avg_total_latency;
        if (interval_time.count() > 1.0) {
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
            std::cout << std::endl;

            // Reset the counters
            last_time_ = high_resolution_clock::now();
            last_query_count_ = 0;
            last_total_latency_ = 0.0;
        }
    }

    void do_connect(ip::tcp::resolver::iterator endpoint_iterator)
    {
        boost::asio::async_connect(socket_, endpoint_iterator,
            [this](boost::system::error_code ec, ip::tcp::resolver::iterator)
            {
                if (!ec)
                {
                    //do_read();
                    do_write();
                }
            });
    }

    void do_read()
    {
        boost::asio::async_read(socket_,
            boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if ((uint32_t)bytes_transferred != packet_size_) {
                    std::cout << "test_latency_client::do_read(): async_read(), bytes_transferred = "
                              << bytes_transferred << " bytes." << std::endl;
                }
                if (!ec)
                {
                    // Have recieved the response message
                    recieve_time_ = high_resolution_clock::now();
                    display_counters();
                    do_write();
                }
            });
    }

    void do_write()
    {
        // Prepare to send the request message
        send_time_ = high_resolution_clock::now();
        boost::asio::async_write(socket_,
            boost::asio::buffer(data_, packet_size_),
            [this](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if ((uint32_t)bytes_transferred != packet_size_) {
                    std::cout << "test_latency_client::do_write(): async_write(), bytes_transferred = "
                              << bytes_transferred << " bytes." << std::endl;
                }
                if (!ec)
                {
                    do_read();
                }
            });
    }
};

} // namespace asio_test
