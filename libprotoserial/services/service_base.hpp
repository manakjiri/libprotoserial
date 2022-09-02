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
    class service_base
    {
        public:
        using port_type = ports_handler::port_type;

        protected:
        port_type _port;

        public:
        /* default construct the service, you need to assign a port number to later */
        service_base() :
            _port(0) {}

        /* registers the service with the ports_handler and binds to it */
        service_base(ports_handler & l, port_type port)
        {
            bind_to(l, port);
        }

        void bind_to(ports_handler & l, port_type port)
        {
            auto & h = l.register_port((_port = port));
            h.receive_event.subscribe(&service_base::receive, this);
            transmit_event.subscribe(&ports_handler::service_endpoint::transmit_callback, &h);
        }

        protected:

        subject<packet> transmit_event;
        /* the service receives a finished packet here */
        virtual void receive(packet) = 0;

        /* the service sends a packet through this */
        void transmit(packet p)
        {
            /* set the port if not set */
            if (!p.source_port())
                p.set_source_port(get_port());
            
            transmit_event.emit(std::move(p));
        }

        port_type get_port() const {return _port;}

    };
}


#endif



