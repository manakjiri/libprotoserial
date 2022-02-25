
#ifndef _SP_INTERFACE
#define _SP_INTERFACE

#include "libprotoserial/interface/loopback.hpp"
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/footers.hpp"


namespace sp
{
    class loopback_interface : 
        public detail::loopback_interface<sp::headers::header, sp::footers::footer> 
    {
        using detail::loopback_interface<sp::headers::header, sp::footers::footer>::loopback_interface;
    };
}

#endif
