
#ifndef _SP_INTERFACE_HEADERS
#define _SP_INTERFACE_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) header_8b8b
        {
            typedef std::uint8_t        address_type;
            typedef std::uint8_t        size_type;

            address_type destination = 0;
            address_type source = 0;
            size_type size = 0;
            byte check = (byte)0;

            header_8b8b() = default;
            header_8b8b(const interface::packet & p):
                destination(p.destination()), source(p.source()), size(p.data().size())
            {
                check = (byte)(destination + source + size);
            }

            bool is_valid(size_type max_size) const 
            {
                return check == (byte)(destination + source + size) && size > 0 && size <= max_size && destination != source;
            }
        };

        struct __attribute__ ((__packed__)) fragment_header_8b16b
        {
            typedef std::uint8_t         index_type;
            typedef std::uint16_t        id_type;

            enum message_types: std::uint8_t
            {
                INIT = 0,
                PACKET,
                RETRANSMIT_REQUEST,
                STATUS_BROADCAST
            };

            message_types type = INIT;
            index_type fragment = 0;
            index_type fragments_total = 0;
            id_type id = 0;
            byte check = (byte)0;

            fragment_header_8b16b() = default;
            fragment_header_8b16b(message_types _type, index_type _fragment, index_type _fragments_total, id_type _id):
                type(_type), fragment(_fragment), fragments_total(_fragments_total), id(_id)
            {
                check = (byte)(type + fragment + fragments_total + id);
            }

            bool is_valid() const 
            {
                return check == (byte)(type + fragment + fragments_total + id) && fragment <= fragments_total;
            }
        };
    }
}

#endif
