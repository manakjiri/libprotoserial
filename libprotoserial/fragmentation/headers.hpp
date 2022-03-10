
#ifndef _SP_FRAGMENTATION_HEADERS
#define _SP_FRAGMENTATION_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) fragment_header_8b16b
        {
            typedef std::uint8_t         index_type;
            typedef std::uint16_t        id_type;

            enum message_types: std::uint8_t
            {
                INIT = 0,
                PACKET,
                PACKET_ACK,
                PACKET_REQ,
            };

            fragment_header_8b16b() = default;
            fragment_header_8b16b(message_types type, index_type fragment, index_type fragments_total, id_type id, id_type prev_id = 0):
                _type(type), _fragment(fragment), _fragments_total(fragments_total), _id(id), _prev_id(prev_id)
            {
                _check = (byte)(_type + _fragment + _fragments_total + _id + _prev_id);
            }

            message_types type() const {return _type;}
            index_type fragment() const {return _fragment;}
            index_type fragments_total() const {return _fragments_total;}
            id_type get_id() const {return _id;}
            id_type get_prev_id() const {return _prev_id;}
            
            /* only basic sanity checks are performed, type is not checked, here we assume that this is a secondary
            header, it should already be checksummed by the lower layer, the only real reason for this check
            is the case where the contra-peer does not use this header when we expect it */
            bool is_valid() const 
            {
                return _check == (byte)(_type + _fragment + _fragments_total + _id + _prev_id) && _fragment != 0 && _fragment <= _fragments_total;
            }

            private:
            message_types _type = INIT;
            index_type _fragment = 0;
            index_type _fragments_total = 0;
            id_type _id = 0;
            id_type _prev_id = 0;
            byte _check = (byte)0;
        };
    }
}

#endif