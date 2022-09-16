/*
 * This file is a part of the libprotoserial project
 * https://github.com/georges-circuits/libprotoserial
 * 
 * Copyright (C) 2022 Jiří Maňák - All Rights Reserved
 * For contact information visit https://manakjiri.eu/
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/gpl.html>
 */

#ifndef _SP_INTERFACE
#define _SP_INTERFACE

#include "libprotoserial/libconfig.hpp"

#include "libprotoserial/interface/testing/loopback.hpp"
#include "libprotoserial/interface/testing/virtual.hpp"
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/footers.hpp"

#ifdef SP_STM32
#include "libprotoserial/interface/stm32/uart.hpp"
#include "libprotoserial/interface/stm32/usbcdc.hpp"
#endif

#ifdef SP_STM32ZST
#include "libprotoserial/interface/stm32_zst/uart.hpp"
#include "libprotoserial/interface/stm32_zst/usbcdc.hpp"
#endif

#ifdef SP_LINUX
#include "libprotoserial/interface/linux/uart.hpp"
#endif

namespace sp
{
    class loopback_interface : 
        public detail::loopback_interface<sp::headers::interface_8b8b, sp::footers::crc32> 
    {
        using detail::loopback_interface<sp::headers::interface_8b8b, sp::footers::crc32>::loopback_interface;
    };

    class virtual_interface : 
        public detail::virtual_interface<sp::headers::interface_8b8b, sp::footers::crc32> 
    {
        using detail::virtual_interface<sp::headers::interface_8b8b, sp::footers::crc32>::virtual_interface;
    };


#if defined(SP_STM32ZST)
    namespace env = detail::stm32_zst;
#elif defined(SP_STM32)
    namespace env = detail::stm32;
#elif defined(SP_LINUX)
    namespace env = detail::pc;
#endif

#if defined(SP_EMBEDDED) || defined(SP_LINUX)
#define SP_UART_AVAILABLE
    class uart_interface:
    	public env::uart_interface<sp::headers::interface_8b8b, sp::footers::crc32>
    {
    	using env::uart_interface<sp::headers::interface_8b8b, sp::footers::crc32>::uart_interface;
    };

#define SP_USBCDC_AVAILABLE
    class usbcdc_interface:
        public env::usbcdc_interface<sp::headers::interface_8b8b, sp::footers::crc32>
    {
        using env::usbcdc_interface<sp::headers::interface_8b8b, sp::footers::crc32>::usbcdc_interface;
    };
#endif

}

#endif
