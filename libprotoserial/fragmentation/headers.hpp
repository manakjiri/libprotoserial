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

#ifndef _SP_FRAGMENTATION_HEADERS
#define _SP_FRAGMENTATION_HEADERS

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace headers
    {
        struct __attribute__ ((__packed__)) fragment_8b8b
        {
            typedef std::uint8_t        index_type;
            typedef std::uint8_t        id_type;
            typedef std::uint8_t        status_type;

            enum message_types: std::uint8_t
            {
                INIT = 0,
                FRAGMENT,
                FRAGMENT_ACK,
                FRAGMENT_REQ,
            };

            fragment_8b8b() = default;
            fragment_8b8b(message_types type, index_type fragment, index_type fragments_total, id_type id, id_type prev_id, status_type status):
                _type(type), _fragment(fragment), _fragments_total(fragments_total), _id(id), _prev_id(prev_id), _status(status)
            {
                _check = (byte)(_type + _fragment + _fragments_total + _id + _prev_id + _status);
            }

            message_types type() const {return _type;}
            index_type fragment() const {return _fragment;}
            index_type fragments_total() const {return _fragments_total;}
            id_type get_id() const {return _id;}
            id_type get_prev_id() const {return _prev_id;}
            status_type status() const {return _status;}
            
            /* only basic sanity checks are performed, type is not checked, here we assume that this is a secondary
            header, it should already be checksummed by the lower layer, the only real reason for this check
            is the case where the contra-peer does not use this header when we expect it */
            bool is_valid() const 
            {
                return _check == (byte)(_type + _fragment + _fragments_total + _id + _prev_id + _status) && 
                    _fragment != 0 && _fragments_total != 0 && _fragment <= _fragments_total && _id != 0;
            }

            /* struct status
            {
                status(const fragment_8b8b & h) : value(h.status()) {}
                status(const interface::status & s, const configuration & c)
                {
                    //TODO in handler
                    value = 0;
                    if (s.receive_buffer_level >= c.frb_critical)
                        value |= 0x03;
                    else if (s.receive_buffer_level >= c.frb_poor)
                        value |= 0x01;
                }
                
                bool rx_poor() {return (value & 0x01) == 0x01;}
                bool rx_critical() {return (value & 0x03) == 0x03;}
                
                private:
                status_type value;
            }; */

            private:
            message_types _type = INIT;
            index_type _fragment = 0;
            index_type _fragments_total = 0;
            id_type _id = 0;
            id_type _prev_id = 0;
            status_type _status = 0;
            byte _check = (byte)0;
        };
    }
}

#endif
