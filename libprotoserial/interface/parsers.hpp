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

#include <optional>

namespace sp
{
    namespace parsers
    {
        template<typename header, typename footer>
        std::optional<fragment> parse_fragment(bytes && buff, const interface & i)
        {
            /* copy the header into the header struct */
            header h;
            std::copy(buff.begin(), buff.begin() + sizeof(h), reinterpret_cast<byte*>(&h));
            
            if (!h.is_valid(i.max_data_size()))
                return std::nullopt;
            
            /* copy the footer, shrink the container by the footer size and compute the checksum */
            footer f_parsed;
            std::copy(buff.end() - sizeof(footer), buff.end(), reinterpret_cast<byte*>(&f_parsed));
            buff.shrink(0, sizeof(footer));
            footer f_computed(buff);
            
            if (f_parsed.hash != f_computed.hash)
                return std::nullopt;
            
            /* shrink the container by the header and return the fragment object */
            buff.shrink(sizeof(h), 0);
            return fragment(interface::address_type(h.source), interface::address_type(h.destination), 
                std::move(buff), i.interface_id());
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
        /* iterator needs to be able to increment at least sizeof(Target) times */
        template<typename Target, typename Iterator>
        Target byte_copy(const Iterator & start)
        {
            Target t;
            Iterator it = start;
            for (uint pos = 0; pos < sizeof(t); ++it, ++pos)
                reinterpret_cast<byte*>(&t)[pos] = *it;
            return t;
        }
        /* bytes byte_copy(const detail::buffered_interface::circular_iterator & start, 
            const detail::buffered_interface::circular_iterator & end)
        {
            bytes b(distance(start, end));
            auto it = start;
            for (uint pos = 0; pos < b.size() && it != end; ++it, ++pos)
                b[pos] = *it;
            return b;
        } */
    }
}

#endif
