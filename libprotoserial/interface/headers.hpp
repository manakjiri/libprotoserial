
#ifndef _SP_INTERFACE_HEADERS
#define _SP_INTERFACE_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) header
        {
            typedef std::uint8_t                address_type;
            typedef std::uint8_t                size_type;

            address_type destination = 0;
            address_type source = 0;
            size_type size_check = 0;
            size_type size = 0;

            header() = default;
            header(const interface::packet & p):
                destination(p.destination()), source(p.source()), size(p.data().size())
                {size_check = ~size;}

            bool is_size_valid() const {return size == (size_type)(~size_check) && size > 0;}
        };
    }
}

#endif
