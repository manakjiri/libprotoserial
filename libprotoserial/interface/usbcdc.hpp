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

#ifndef _SP_INTERFACE_USBCDC
#define _SP_INTERFACE_USBCDC

#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    namespace detail
    {
        template<class header, class footer>
        class usbcdc_interface : public buffered_interface
        {
            public: 

            /* PACKET STRUCTURE: [header][data >= 1][footer] */

            /* this is the USB CDC interface yet is uses "usb%n" as its name, if there is some 
            other type of USB interface in the future, be wary of this */
            usbcdc_interface(uint id, interface::address_type address, uint max_queue_size):
                interface("usb" + std::to_string(id), address, max_queue_size) {}

            
            bytes::size_type max_data_size() const noexcept {return _max_packet_size - sizeof(header) - sizeof(footer);}
            bool can_transmit() noexcept 
            {
                return true; //TODO
            }

            void do_receive()
            {
                //TODO
            }
            
            bytes serialize_packet(packet && p) const 
            {
                if (p.data().size() > max_data_size()) throw data_too_long();
                /* preallocate the container since we know the final size */
                auto b = bytes(0, 0, sizeof(header) + p.data().size() + sizeof(footer));
                /* header */
                b.push_back(to_bytes(header(p)));
                /* data */
                b.push_back(p.data());
                /* footer */
                b.push_back(to_bytes(footer(b.begin(), b.end())));
                return b;
            }

            bool do_transmit(bytes && buff) noexcept 
            {
                //TODO
            }

            private:
            const uint _max_packet_size = 64;
        };
    }

} // namespace sp
#endif
