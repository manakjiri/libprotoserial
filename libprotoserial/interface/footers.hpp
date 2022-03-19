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

#ifndef _SP_INTERFACE_FOOTERS
#define _SP_INTERFACE_FOOTERS

#include "libprotoserial/interface/interface.hpp"

#include "submodules/etl/include/etl/crc32.h"
#include "submodules/etl/include/etl/crc16.h"

namespace sp
{
    namespace footers
    {
        struct __attribute__ ((__packed__)) crc32
        {
            typedef etl::crc32                  hash_algorithm;
            typedef hash_algorithm::value_type  hash_type;

            hash_type hash = 0;

            crc32() = default;
            crc32(bytes::iterator begin, bytes::iterator end):
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(begin), 
                    reinterpret_cast<const uint8_t*>(end)).value()) {}
            crc32(const bytes & b) :
                crc32(b.begin(), b.end()) {}
        };

        struct __attribute__ ((__packed__)) crc16
        {
            typedef etl::crc16                  hash_algorithm;
            typedef hash_algorithm::value_type  hash_type;

            hash_type hash = 0;

            crc16() = default;
            crc16(bytes::iterator begin, bytes::iterator end):
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(begin), 
                    reinterpret_cast<const uint8_t*>(end)).value()) {}
            crc16(const bytes & b) :
                crc16(b.begin(), b.end()) {}
        };
    }
}

#endif
