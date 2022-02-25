
#ifndef _SP_INTERFACE_FOOTERS
#define _SP_INTERFACE_FOOTERS

#include "libprotoserial/interface/interface.hpp"
#include "submodules/etl/include/etl/crc32.h"

namespace sp
{
    namespace footers
    {
        struct __attribute__ ((__packed__)) footer
        {
            typedef etl::crc32                  hash_algorithm;
            typedef hash_algorithm::value_type  hash_type;

            hash_type hash = 0;

            footer() = default;
            footer(bytes::iterator begin, bytes::iterator end):
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(begin), 
                    reinterpret_cast<const uint8_t*>(end)).value()) {}
            footer(const bytes & b) :
                footer(b.begin(), b.end()) {}
        };
    }
}

#endif
