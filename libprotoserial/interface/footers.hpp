
#ifndef _SP_INTERFACE_FOOTERS
#define _SP_INTERFACE_FOOTERS

#include "libprotoserial/interface/interface.hpp"

#include "submodules/etl/include/etl/crc32.h"
#include "submodules/etl/include/etl/crc16.h"

namespace sp
{
    namespace footers
    {
        struct __attribute__ ((__packed__)) footer_crc32
        {
            typedef etl::crc32                  hash_algorithm;
            typedef hash_algorithm::value_type  hash_type;

            hash_type hash = 0;

            footer_crc32() = default;
            footer_crc32(bytes::iterator begin, bytes::iterator end):
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(begin), 
                    reinterpret_cast<const uint8_t*>(end)).value()) {}
            footer_crc32(const bytes & b) :
                footer_crc32(b.begin(), b.end()) {}
        };

        struct __attribute__ ((__packed__)) footer_crc16
        {
            typedef etl::crc16                  hash_algorithm;
            typedef hash_algorithm::value_type  hash_type;

            hash_type hash = 0;

            footer_crc16() = default;
            footer_crc16(bytes::iterator begin, bytes::iterator end):
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(begin), 
                    reinterpret_cast<const uint8_t*>(end)).value()) {}
            footer_crc16(const bytes & b) :
                footer_crc16(b.begin(), b.end()) {}
        };
    }
}

#endif
