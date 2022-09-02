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


#ifndef _SP_SERVICES_SERVICEBASE
#define _SP_SERVICES_SERVICEBASE

#include "libprotoserial/utils/observer.hpp"
#include "libprotoserial/ports/ports.hpp"

namespace sp
{
    /* this base class is not required for a service, it is just a "template" for built-in services */
    class port_service_base
    {
        public:
        using port_type = ports_handler::port_type;

        protected:
        port_type _port;

        public:

        /*  */
        subject<packet> transmit_event;
        virtual void receive_callback(packet) = 0;

        void bind_to(ports_handler & l, port_type port)
        {
            auto & h = l.register_port((_port = port));
            h.receive_event.subscribe(&port_service_base::receive_callback, this);
            transmit_event.subscribe(&ports_handler::service_endpoint::transmit_callback, &h);
        }

        port_type get_port() const {return _port;}

    };
}


#endif



