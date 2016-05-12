
#pragma once

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "common.h"

#define MAX_PACKET_SIZE	    65536

using namespace boost::asio;

namespace asio_test {

class test_pingpong_client
{
private:
    enum { PACKET_SIZE = MAX_PACKET_SIZE };
    
    boost::asio::io_service & io_service_;
    ip::tcp::socket socket_;
    uint32_t packet_size_;
    char data_[PACKET_SIZE];

public:
    test_pingpong_client(boost::asio::io_service & io_service,
        ip::tcp::resolver::iterator endpoint_iterator, uint32_t packet_size)
        : io_service_(io_service),
          socket_(io_service), packet_size_(packet_size)
    {
        ::memset(data_, 'h', sizeof(data_));
        do_connect(endpoint_iterator);
    }

    ~test_pingpong_client()
    {
        //
    }

private:
    void do_connect(ip::tcp::resolver::iterator endpoint_iterator)
    {
        boost::asio::async_connect(socket_, endpoint_iterator,
            [this](const boost::system::error_code & ec, ip::tcp::resolver::iterator)
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
            [this](const boost::system::error_code & ec, std::size_t /*bytes_transferred*/)
            {
                if (!ec)
                {
                    do_write();
                }
            });
    }

    void do_write()
    {
        boost::asio::async_write(socket_,
            boost::asio::buffer(data_, packet_size_),
            [this](const boost::system::error_code & ec, std::size_t /*bytes_transferred*/)
            {
                if (!ec)
                {
                    do_read();
                }
            });
    }
};

} // namespace asio_test
