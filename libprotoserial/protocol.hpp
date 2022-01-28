#include <libprotoserial/stream.hpp>

#include <array>

namespace sp
{
    template<uint8_t _port_count>
    class ports : public stream
    {
        protected:
        class port : public stream
        {
            uint8_t _id;

            port(std::shared_ptr<consumer> c, uint8_t id)
            {
                _id = id;
                bind(c);
            }
            
            int write(std::unique_ptr<bytes> && data)
            {
                int len = data.get()->size();
                dispatch(std::move(data));
                return len;
            }

            bool writable() 
            {
                return true;
            }
        };
 
        std::array<port, _port_count> ports;

        public:
        int write(std::unique_ptr<bytes> && data)
        {

        }
        
        bool writable() 
        {
            return true;
        }

    };
}
