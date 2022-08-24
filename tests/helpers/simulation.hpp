
#include "libprotoserial/interface.hpp"
#include "libprotoserial/utils/bit_rate.hpp"
#include "libprotoserial/utils/observer.hpp"

#include <thread>
#include <mutex>
#include <queue>
#include <array>
#include <atomic>

namespace sim
{
    class loop_thread
    {
        std::function<void(void)> _fn;
        std::jthread _thread;
        std::atomic<bool> _run;

        void callback()
        {
            while (!_run)
                ;
            while (_run)
            {
                _fn();
            }
        }

    public:
        // TODO args
        loop_thread(std::function<void(void)> fn) : _fn(fn), _thread(std::bind(&loop_thread::callback, this)), _run(true) {}
        ~loop_thread() { join(); }

        void join()
        {
            if (_run)
            {
                _run = false;
                _thread.join();
            }
        }
    };

    template <std::size_t N>
    class wire
    {
    public:
        std::array<sp::subject<sp::byte>, N> receive_events;

    protected:
        /* queue for each endpoint, the put function fills these queues,
        single_callback dispatches the data byte-by-byte through receive_events */
        std::array<std::queue<sp::byte>, N> _endpoints;

        virtual void dispatch() = 0;

    private:
        loop_thread _thread;
        std::mutex _endpoint_mutex;
        sp::bit_rate _rate;

        void callback()
        {
            // std::cout << "callback lock" << std::endl;
            _endpoint_mutex.lock();
            dispatch();
            // std::cout << "callback unlock" << std::endl;
            _endpoint_mutex.unlock();
            std::this_thread::sleep_for(_rate.bit_period() * 8);
        }

    public:
        wire(sp::bit_rate rate) : _thread(std::bind(&wire<N>::callback, this)), _rate(rate) {}

        void put(std::size_t origin, sp::byte data)
        {
            const std::lock_guard<std::mutex> lock(_endpoint_mutex);
            for (std::size_t i = 0; i < N; ++i)
            {
                if (i != origin)
                    _endpoints.at(i).push(data);
            }
        }

        void put(std::size_t origin, const sp::bytes &data)
        {
            // std::cout << "put lock" << std::endl;
            const std::lock_guard<std::mutex> lock(_endpoint_mutex);
            for (std::size_t i = 0; i < N; ++i)
            {
                if (i != origin)
                {
                    for (auto d : data)
                        _endpoints.at(i).push(d);
                }
            }
            // std::cout << "put unlock" << std::endl;
        }

        void stop()
        {
            _thread.join();
        }
    };

    template <std::size_t N>
    class fullduplex : public wire<N>
    {
        void dispatch()
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                if (!this->_endpoints.at(i).empty())
                {
                    this->receive_events.at(i).emit(this->_endpoints.at(i).front());
                    this->_endpoints.at(i).pop();
                }
            }
        }

    public:
        ~fullduplex()
        {
            this->stop();
        }

        using wire<N>::wire;
    };
}
