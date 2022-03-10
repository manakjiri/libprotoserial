
#ifndef _SP_INTERFACE_HEADERS
#define _SP_INTERFACE_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) interface_header_8b8b
        {
            typedef std::uint8_t        address_type;
            typedef std::uint8_t        size_type;

            address_type destination = 0;
            address_type source = 0;
            size_type size = 0;
            byte check = (byte)0;

            interface_header_8b8b() = default;
            interface_header_8b8b(const interface::packet & p):
                destination(p.destination()), source(p.source()), size(p.data().size())
            {
                check = (byte)(destination + source + size);
            }

            bool is_valid(size_type max_size) const 
            {
                return check == (byte)(destination + source + size) && size > 0 && size <= max_size && destination != source;
            }
        };
    }
}

#endif
