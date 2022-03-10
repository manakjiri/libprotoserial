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
