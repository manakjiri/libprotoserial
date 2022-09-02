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


#ifndef _SP_SERVICES_LINKCONTROL
#define _SP_SERVICES_LINKCONTROL

#include "libprotoserial/services/service_base.hpp"

namespace sp
{
namespace services
{
    class link_control : public port_service_base
    {
        enum message_types: std::uint8_t
        {
            PING_REQ = 1,
            PING_RESP,
        };

        void transfer_receive_callback(packet p)
        {
            message_types type = static_cast<message_types>(*p.data_begin());
            p.remove_first_n(sizeof(message_types));
            
            switch (type)
            {
            case message_types::PING_REQ:
                p.push_front(to_bytes(static_cast<byte>(message_types::PING_RESP)));
                transmit_event.emit(std::move(packet(p.create_response(), std::move(p))));
                break;
            
            default:
                break;
            }
        }


        public:

        void ping(interface::address_type addr, interface_identifier iid, port_type port = 0, bytes data = bytes())
        {
            if (!port) 
                port = get_port();

            packet p()
        }

    };
}
}


#endif
