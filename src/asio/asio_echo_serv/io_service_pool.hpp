
#pragma once

#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>

using namespace boost::asio;

namespace asio_test {

class io_service_pool : private boost::noncopyable {
private:
    typedef boost::shared_ptr<boost::asio::io_service>			io_service_ptr;
    typedef boost::shared_ptr<boost::asio::io_service::work>	io_work_ptr;

    /// The pool of io_services.
    std::vector<io_service_ptr> io_services_;

    /// The work that keeps the io_services running.
    std::vector<io_work_ptr> workes_;

    /// The next io_service to use for a connection.
    std::atomic<uint32_t> next_io_service_;

public:
    /// Construct the io_service pool.
    explicit io_service_pool(uint32_t pool_size)
        : next_io_service_(0)
    {
        if (pool_size == 0)
            throw std::runtime_error("io_service_pool size is 0.");

        // Give all the io_services work to do so that their run() functions will not
        // exit until they are explicitly stopped.
        for (uint32_t i = 0; i < pool_size; ++i) {
            io_service_ptr io_service(new boost::asio::io_service);
            io_work_ptr work(new boost::asio::io_service::work(*io_service));
            io_services_.push_back(io_service);
            workes_.push_back(work);
        }
    }

    ~io_service_pool()
    {
        stop();
    }

    /// Run all io_service objects in the pool.
    void run()
    {
        // Create a pool of threads to run all of the io_services.
        std::vector< boost::shared_ptr<boost::thread> > threads;
        for (std::size_t i = 0; i < io_services_.size(); ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(
                boost::bind(&boost::asio::io_service::run, io_services_[i])));
            threads.push_back(thread);
        }

        // Wait for all threads in the pool to exit.
        for (std::size_t i = 0; i < threads.size(); ++i)
            threads[i]->join();
    }

    /// Stop all io_service objects in the pool.
    void stop()
    {
        // Explicitly stop all io_services.
        for (std::size_t i = 0; i < io_services_.size(); ++i)
            io_services_[i]->stop();
    }

    /// Get an io_service to use.
    boost::asio::io_service & get_io_service()
    {
        // Use a round-robin scheme to choose the next io_service to use.
        boost::asio::io_service & io_service = *io_services_[next_io_service_];
        next_io_service_++;
        if (next_io_service_ >= io_services_.size())
            next_io_service_ = 0;
        return io_service;
    }

    /// Get an io_service to use.
    boost::asio::io_service & get_now_io_service()
    {
        // Use a round-robin scheme to choose the next io_service to use.
        if (next_io_service_ >= io_services_.size())
            next_io_service_ = 0;
        boost::asio::io_service & io_service = *io_services_[next_io_service_];
        return io_service;
    }

    /// Get an io_service to use.
    boost::asio::io_service & get_first_io_service()
    {
        // Use a round-robin scheme to choose the next io_service to use.
        boost::asio::io_service & io_service = *io_services_[0];
        return io_service;
    }
};

} // namespace asio_test
