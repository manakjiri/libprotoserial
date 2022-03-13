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

#ifndef _SP_INTERFACE_PARSERS
#define _SP_INTERFACE_PARSERS

#include "libprotoserial/interface/buffered.hpp"

#include <stdexcept>

namespace sp
{
    namespace parsers
    {
        struct bad_checksum : std::exception {
            const char * what () const throw () {return "bad_checksum";}
        };

        struct bad_size : std::exception {
            const char * what () const throw () {return "bad_size";}
        };

        template<typename header, typename footer>
        interface::packet parse_packet(bytes && buff, const interface * i)
        {
            bytes b = buff;
            /* copy the header into the header struct */
            header h;
            std::copy(b.begin(), b.begin() + sizeof(h), reinterpret_cast<byte*>(&h));
            if (!h.is_valid(i->max_data_size())) throw bad_size();
            /* copy the footer, shrink the container by the footer size and compute the checksum */
            footer f_parsed;
            std::copy(b.end() - sizeof(footer), b.end(), reinterpret_cast<byte*>(&f_parsed));
            b.shrink(0, sizeof(footer));
            footer f_computed(b);
            if (f_parsed.hash != f_computed.hash) throw bad_checksum();
            /* shrink the container by the header and return the packet object */
            b.shrink(sizeof(h), 0);
            return interface::packet(interface::address_type(h.source), interface::address_type(h.destination), 
                std::move(b), i);
        }

        /* find the value by incrementing start, if found returns true, false otherwise */
        template<typename Iterator, typename T>
        bool find(Iterator & start, const Iterator & end, T value)
        {
            for (; start != end; ++start)
            {
                if (*start == value)
                    return true;
            }
            return false;
        }
        
        
        template<typename Target, typename Iterator>
        Target byte_copy(const Iterator & start, const Iterator & end)
        {
            Target t;
            Iterator it = start;
            for (uint pos = 0; pos < sizeof(t) && it != end; ++it, ++pos)
                reinterpret_cast<byte*>(&t)[pos] = *it;
            return t;
        }
        bytes byte_copy(const detail::buffered_interface::circular_iterator & start, 
            const detail::buffered_interface::circular_iterator & end)
        {
            bytes b(distance(start, end));
            auto it = start;
            for (uint pos = 0; pos < b.size() && it != end; ++it, ++pos)
                b[pos] = *it;
            return b;
        }
    }
}

#endif
