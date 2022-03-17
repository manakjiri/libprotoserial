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

#ifndef _SP_INTERFACE_HEADERS
#define _SP_INTERFACE_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) interface_8b8b
        {
            typedef std::uint8_t        address_type;
            typedef std::uint8_t        size_type;

            address_type destination = 0;
            address_type source = 0;
            size_type size = 0;
            byte check = (byte)0;

            interface_8b8b() = default;
            interface_8b8b(const interface::fragment & p):
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
