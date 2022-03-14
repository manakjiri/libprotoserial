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

#ifndef _SP_PORTS_HEADERS
#define _SP_PORTS_HEADERS

#include "libprotoserial/byte.hpp"

#include <cstdint>

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) ports_8b
        {
            typedef std::uint8_t        port_type;

            port_type destination = 0;
            port_type source = 0;
            byte check = (byte)0;

            ports_8b() = default;
            ports_8b(port_type dst, port_type src):
                destination(dst), source(src)
            {
                check = (byte)(destination + source);
            }

            bool is_valid() const 
            {
                return check == (byte)(destination + source) && destination != source 
                    && destination && source;
            }
        };
    }
}

#endif
