
#ifndef _SP_INTERFACE
#define _SP_INTERFACE

#include "libprotoserial/interface/loopback.hpp"
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/footers.hpp"


namespace sp
{
    class loopback_interface : 
        public detail::loopback_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32> 
    {
        using detail::loopback_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32>::loopback_interface;
    };
}

#endif
