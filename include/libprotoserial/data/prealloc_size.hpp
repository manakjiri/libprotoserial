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


#ifndef _SP_DATA_PREALLOCSIZE
#define _SP_DATA_PREALLOCSIZE

#include <libprotoserial/data/container.hpp>

namespace sp
{
    class prealloc_size
    {
        bytes::size_type _front, _back;

        public:

        constexpr prealloc_size(bytes::size_type front, bytes::size_type back) :
            _front(front), _back(back) {}

        constexpr prealloc_size() : 
            prealloc_size(0, 0) {}

        constexpr prealloc_size(const prealloc_size &) = default;
        constexpr prealloc_size(prealloc_size &&) = default;
        constexpr prealloc_size & operator=(const prealloc_size &) = default;
        constexpr prealloc_size & operator=(prealloc_size &&) = default;

        constexpr auto front() const {return _front;}
        constexpr auto back() const {return _back;}
        
        inline auto create(bytes::size_type length) const 
        {
            return bytes(front(), length, back());
        }
        inline auto create(bytes::size_type add_front, bytes::size_type length, bytes::size_type add_back) const
        {
            return bytes(front() + add_front, length, back() + add_back);
        }

        friend constexpr prealloc_size operator+(const prealloc_size & lhs, const prealloc_size & rhs)
		{
        	return prealloc_size(lhs.front() + rhs.front(), lhs.back() + rhs.back());
		}
    };
}

#endif


