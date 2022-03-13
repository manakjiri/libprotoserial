
#ifndef _SP_INTERFACE
#define _SP_INTERFACE

#include "libprotoserial/libconfig.hpp"

#include "libprotoserial/interface/loopback.hpp"
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/footers.hpp"

#ifdef SP_STM32
#include "libprotoserial/interface/stm32/uart.hpp"
#include "libprotoserial/interface/stm32/usbcdc.hpp"
#endif

namespace sp
{
    class loopback_interface : 
        public detail::loopback_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32> 
    {
        using detail::loopback_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32>::loopback_interface;
    };


#ifdef SP_STM32
    namespace device = detail::stm32;
#endif

#ifdef SP_EMBEDDED
    class uart_interface:
    	public device::uart_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32>
    {
    	using device::uart_interface<sp::headers::interface_header_8b8b, sp::footers::footer_crc32>::uart_interface;
    };
#endif

}

#endif
