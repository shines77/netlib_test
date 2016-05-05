
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
#include "connection.hpp"

using namespace boost::asio;

namespace asio_test {

//
// See: http://www.boost.org/doc/libs/1_36_0/doc/html/boost_asio/example/echo/async_tcp_echo_server.cpp
//
class async_asio_echo_serv : public boost::enable_shared_from_this<async_asio_echo_serv>,
							 private boost::noncopyable
{
public:
	async_asio_echo_serv(const std::string & ip_addr, const std::string & port,
		std::uint32_t packet_size = 64,
		std::uint32_t pool_size = std::thread::hardware_concurrency(),
		boost::system::error_code & ec = boost::system::error_code())
		: io_service_pool_(pool_size), packet_size_(packet_size), acceptor_(io_service_pool_.get_io_service())
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

	async_asio_echo_serv(short port, std::uint32_t packet_size = 64, std::uint32_t pool_size = std::thread::hardware_concurrency())
		: io_service_pool_(pool_size), packet_size_(packet_size),
		  acceptor_(io_service_pool_.get_io_service(), ip::tcp::endpoint(ip::tcp::v4(), port))
	{
		do_accept();
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
	void handle_accept(const boost::system::error_code & ec, connection * conn)
	{
		if (!ec) {
			if (conn) {
				conn->start();
			}
		}
		else {
			// Write error log
			std::cout << "async_asio_echo_serv::handle_accept() - Error: (code = " << ec.value() << ") "
						<< ec.message().c_str() << std::endl;
			if (conn) {
				delete conn;
			}
		}

		do_accept();
	}

	void do_accept()
	{
		boost::system::error_code & ec = s_ec;
		//boost::shared_ptr<connection> new_conn(new connection(io_service_pool_.get_io_service()));
		//boost::shared_ptr<connection> new_conn = connection::create_new(io_service_pool_.get_io_service());
		connection * new_conn = new connection(io_service_pool_.get_io_service(), packet_size_);
		acceptor_.async_accept(new_conn->socket(), boost::bind(&async_asio_echo_serv::handle_accept,
			this, boost::asio::placeholders::error, new_conn));
	}

	void do_accept2()
	{
		conn_.reset(new connection(io_service_pool_.get_io_service(), packet_size_));
		acceptor_.async_accept(conn_->socket(),
			[this](boost::system::error_code ec)
			{
				if (!ec) {
					conn_->start();
				}

				do_accept2();
			});
	}

public:
	static boost::system::error_code s_ec;

private:
	io_service_pool					io_service_pool_;
	boost::asio::ip::tcp::acceptor	acceptor_;
	std::shared_ptr<connection>		conn_;
	std::shared_ptr<std::thread>	thread_;
	std::uint32_t					packet_size_;
};

boost::system::error_code async_asio_echo_serv::s_ec;

} // namespace asio_test
