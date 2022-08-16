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

#include "libprotoserial/interface/buffered.hpp"

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
        class loopback_interface : public buffered_parser_interface<Header, Footer>
        {
            using parent = buffered_parser_interface<Header, Footer>;

            public: 

            typedef std::function<byte(byte)>   transfer_function;

            /* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

            /* use the wire to implement data in transit corrupting function */
            loopback_interface(interface_identifier::instance_type instance, interface::address_type address, interface::address_type broadcast_address,
                uint max_queue_size, uint max_fragment_size, uint buffer_size, transfer_function wire = [](byte b){return b;}):
                    parent(interface_identifier(interface_identifier::identifier_type::LOOPBACK, instance), 
                    address, broadcast_address, max_queue_size, buffer_size, max_fragment_size), _wire(wire) {}

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
                    this->put_single_received(_wire(*i));
                return true;
            }

            /* this function should be implemented in the parent class, here I need to overload it to do 
            the address swap before the crc gets calculated, that's better than hacking the packet in the 
            do_transmit function */
            bytes serialize_fragment(fragment && p) const 
            {
                /* Header */
                p.data().push_front(to_bytes(Header(p)));
                /* swap the dst and the src address, this obviously 
                only works with the 8b8b Header */
                auto tmp = p.data()[0];
                p.data()[0] = p.data()[1];
                p.data()[1] = tmp;
                /* preamble */
                auto pr = bytes(parent::preamble_length);
                pr.set(parent::preamble);
                p.data().push_front(pr);
                /* Footer */
                p.data().push_back(to_bytes(Footer(
                    p.data().begin() + parent::preamble_length, p.data().end()
                )));
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "serialize_fragment returning: " << b << std::endl;
#endif
                return p.data();
            }

            private:
            transfer_function _wire;
        };
    }
} // namespace sp

#endif
