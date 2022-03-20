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

#ifndef _SP_INTERFACE_VIRTUAL
#define _SP_INTERFACE_VIRTUAL

#include "libprotoserial/interface/buffered_parsed.hpp"

#include <queue>

namespace sp
{
    namespace detail
    {
        /* this class acts purely as a encoder/decoder of fragments
        - subscribe to its receive_event and pass it the raw data to decode using the put_received function
        - use the transmit() function to pass in a fragment, then once has_serialized() returns true you can 
          retrieve the encoded data using the get_serialized() function */
        template<class Header, class Footer>
        class virtual_interface : public buffered_parsed_interface<Header, Footer>
        {
            using parent = buffered_parsed_interface<Header, Footer>;

            public: 

            virtual_interface(interface_identifier::instance_type instance, interface::address_type address, 
                uint max_queue_size, uint max_fragment_size, uint buffer_size):
                    parent(interface_identifier(interface_identifier::identifier_type::VIRTUAL, instance), 
                    address, max_queue_size, buffer_size, max_fragment_size) {}

            bool has_serialized() const {return !_serialized.empty();}
            bytes get_serialized() 
            {
                parent::main_task();
                bytes ret = std::move(_serialized.front());
                _serialized.pop();
                return ret;
            }

            void put_received(bytes && data)
            {
                for (auto & b : data)
                {
                    *(++parent::_write) = b;
                    parent::main_task();
                }
            }

            protected:

            bool can_transmit() noexcept {return true;}
            bool do_transmit(bytes && buff) noexcept 
            {
                parent::main_task();
                _serialized.push(std::move(buff));
                return true;
            }

            std::queue<bytes> _serialized;
        };
    }
} // namespace sp

#endif
