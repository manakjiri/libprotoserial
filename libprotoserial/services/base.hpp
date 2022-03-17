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


#ifndef _SP_SERVICES_BASE
#define _SP_SERVICES_BASE

#include "libprotoserial/observer.hpp"
#include "libprotoserial/ports/ports.hpp"

namespace sp
{
    /* this base class is not required for a service, it is just a "template" 
     * for built-in services and enables a function like */
    struct port_service_base
    {
        using port_type = ports_handler::port_type;

        /* port where to send the provided transfer */
        subject<port_type, transfer> transfer_transmit_event;
        /* source port of the transfer */
        virtual void transfer_receive_callback(port_type p, transfer t) = 0;

        void bind_to(ports_handler & l, port_type port)
        {
            auto & h = l.register_port(_port = port);
            h.transfer_receive_event.subscribe(&port_service_base::transfer_receive_callback, this);
            transfer_transmit_event.subscribe(&ports_handler::service_endpoint::transfer_transmit_callback, &h);
        }

        port_type get_port() const {return _port;}

        private:

        port_type _port;
    };
}


#endif



