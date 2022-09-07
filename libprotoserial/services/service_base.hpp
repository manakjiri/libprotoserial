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

#include <libprotoserial/utils/observer.hpp>
#include <libprotoserial/ports/ports.hpp>
#include <libprotoserial/data/prealloc_size.hpp>

namespace sp
{
    /* this base class is not required for a service, it is used by built-in services
    and can be convenient for users as well. 
    This class has two constructors
    - a default constructor that takes no arguments, but the user needs to then bind 
      (register) their class instance with the ports_handler using the bind_to() function
    - a constructor that takes the ports_handler by reference and binds (registers) 
      the service immediately
    The derived class needs to
    - implement the receive(packet) function (received packets targeted at `get_port()`
      port will be passed into this function)
    - call the transmit(packet) function when it wants to transmit a packet */
    class service_base
    {
        public:
        using port_type = ports_handler::port_type;

        private:
        /* the derived class does not need access to this, since everything is handled by the
        transmit function */
        subject<packet> transmit_event;
        port_type _port;
        prealloc_size _prealloc;

        public:
        /* default construct the service, you need to assign a port number to later 
        using the bind_to function */
        service_base() :
            _port(0) {}

        /* registers the service with the ports_handler and binds to it */
        service_base(ports_handler & l, port_type port)
        {
            bind_to(l, port);
        }

        /* registers the service with the ports_handler and binds to it */
        void bind_to(ports_handler & l, port_type port)
        {
            /* register the specified port with the ports_handler, this throws if already registered */
            auto & h = l.register_service((_port = port));
            h.receive_event.subscribe(&service_base::receive, this);
            transmit_event.subscribe(&ports_handler::service_endpoint::_transmit_callback, &h);
            /* also save the prealloc so that we can honor it and make the stack more efficient */
            _prealloc = l.get_minimum_prealloc();
        }

        /* registers the service with the ports_handler and binds to a random free port */
        void bind_to(ports_handler & l)
        {
            bind_to(l, l.get_free_port());
        }

        port_type get_port() const {return _port;}

        protected:

        /* the service receives a finished packet here */
        virtual void receive(packet) = 0;

        /* the service sends a packet using this function, the packet needs to contain
        - the destination address
        - an interface_id of the transmit interface
        - the destination port
        note: use the create_packet function to achieve optimal performance downstream */
        void transmit(packet p)
        {                
            transmit_event.emit(std::move(p));
        }

        /* use this function to create a new packet to be transmitted
        input the maximum expected data size to make downstream operations
        as efficient as possible
        the returned packet will satisfy packet.data().size() == max_data_size
        note that getting this wrong will not result in a failure, but it will
        produce unnecessary slowdowns */
        packet create_packet(packet::data_type::size_type max_data_size) const
        {
            return packet(packet_metadata(), _prealloc.create(max_data_size));
        }
    };
}


#endif



