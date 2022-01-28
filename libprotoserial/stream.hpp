#include <libprotoserial/container.hpp>

#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>


namespace sp
{
    //using callback = std::function<void(std::unique_ptr<bytes>&&)>;


    class consumer
    {
        
        public:
        //struct not_writable: public std::exception {const char * what () const throw (){return "consumer is not writable";}};

        /* use only once writable() returns true */
        virtual int write(std::unique_ptr<bytes> && data) = 0;
        /* used to signal that the consumer is ready for data */
        virtual bool writable() = 0;
    };

    class producer
    {
        public:
        void bind(std::shared_ptr<consumer> c);
        
        protected:
        /* std::vector<std::shared_ptr<consumer>> consumers;
        std::vector<std::unique_ptr<bytes>> buffers; */

        std::shared_ptr<consumer> _consumer;

        void dispatch(std::unique_ptr<bytes> && data);
    };

    class stream : public consumer, public producer {};

    class loopback_stream : public stream
    {
        public:
        int write(std::unique_ptr<bytes> && data);
        bool writable();
    };

    class cout_stream : public consumer
    {
        public:
        int write(std::unique_ptr<bytes> && data);
        bool writable();
    };
}
