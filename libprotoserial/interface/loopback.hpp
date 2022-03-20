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

#ifndef _SP_INTERFACE_LOOPBACK
#define _SP_INTERFACE_LOOPBACK

#include "libprotoserial/interface/buffered_parsed.hpp"

#ifndef SP_NO_IOSTREAM
//#define SP_LOOPBACK_DEBUG
//#define SP_LOOPBACK_WARNING
#endif

#ifdef SP_LOOPBACK_DEBUG
#define SP_LOOPBACK_WARNING
#endif

namespace sp
{
    namespace detail
    {
        template<class Header, class Footer>
        class loopback_interface : public buffered_parsed_interface<Header, Footer>
        {
            using parent = buffered_parsed_interface<Header, Footer>;

            public: 

            typedef std::function<byte(byte)>   transfer_function;

            /* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

            /* use the wire to implement data in transit corrupting function */
            loopback_interface(interface_identifier::instance_type instance, interface::address_type address, 
                uint max_queue_size, uint max_fragment_size, uint buffer_size, transfer_function wire = [](byte b){return b;}):
                    parent(interface_identifier(interface_identifier::identifier_type::LOOPBACK, instance), 
                    address, max_queue_size, buffer_size, max_fragment_size), _wire(wire) {}

            protected:

            bool can_transmit() noexcept {return true;}
            void write_failed(std::exception & e) 
            {
#ifdef SP_LOOPBACK_WARNING
                std::cout << "write_failed: " << e.what() << std::endl;
#endif
            }

            bool do_transmit(bytes && buff) noexcept 
            {
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "transmit: " << buff << std::endl;
#endif
                for (auto i = buff.begin(); i < buff.end(); i++)
                    rx_isr(_wire(*i));
                return true;
            }

            /* this function should be implemented in the parent class, here I need to overload it to do 
            the address swap before the crc gets calculated, that's better than hacking the packet in the 
            do_transmit function */
            bytes serialize_fragment(fragment && p) const 
            {
            	//TODO we can avoid the copy here by calling reserve and hope that the container has enough capacity
                /* preallocate the container since we know the final size */
                auto b = bytes(0, 0, parent::preamble_length + sizeof(Header) + p.data().size() + sizeof(Footer));
                /* preamble */
                auto pr = bytes(parent::preamble_length);
                pr.set(parent::preamble);
                b.push_back(pr);
                /* Header */
                b.push_back(to_bytes(Header(p)));
                /* data */
                b.push_back(p.data());
                /* Footer */
                /* swap the dst and the src address, this obviously 
                only works with the 8b8b Header */
                auto tmp = b[2];
                b[2] = b[3];
                b[3] = tmp;
                b.push_back(to_bytes(Footer(b.begin() + parent::preamble_length, b.end())));
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "serialize_fragment returning: " << b << std::endl;
#endif
                return b;
            }
            
            void rx_isr(byte b)
            {
                /* the iterator wraps around to the beginning at the end of the buffer */
                *(++parent::_write) = b;
            }

            private:
            transfer_function _wire;
        };
    }
} // namespace sp

#endif
