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

/* 
 * 
 * 
 * 
 */

#ifndef _SP_PORTS_PORTS
#define _SP_PORTS_PORTS

#include "libprotoserial/interface/interface.hpp"
#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/ports/headers.hpp"
#include "libprotoserial/ports/packet.hpp"

#include <list>
#include <algorithm>
#include <stdexcept>

#ifndef SP_NO_IOSTREAM
#include <iostream>
/* it is hard to debug someting that happens every 100 transfers using 
 * debugger alone, these enable different levels of debug prints */
//#define SP_PORTS_DEBUG
//#define SP_PORTS_WARNING
#endif


#ifdef SP_PORTS_DEBUG
#define SP_PORTS_WARNING
#endif

namespace sp
{
    struct already_registered : std::exception {
        const char * what () const throw () {return "already_registered";}
    };

    /* port handler does not make any assumptions as to what a service looks like, meaning there
     * is no base class, just the internal service_endpoint, which houses the events and callbacks */
    class ports_handler
    {
        public:
        
        using Header = headers::ports_8b;
        using port_type = packet::port_type;

        struct service_endpoint
        {
            service_endpoint(port_type p, ports_handler * handler):
                _port(p), _handler(handler) {}

            /* source port of the transfer, service should subscribe to this 
            in order to receive transfers */
            subject<packet> transfer_receive_event;

            /* port where to send the provided transfer, service should call this
            when it wants to transmit a transfer, creating a subject<port_type, transfer>
            within the service and subscribing using this fuction is recommended */
            void transfer_transmit_callback(packet p)
            {
                _handler->transfer_transmit(_port, std::move(p));
            }

            auto get_port() const {return _port;}
            
            private:
            ports_handler * _handler;
            port_type _port; 
        };

        struct interface_endpoint
        {
            interface_endpoint(interface_identifier iid, ports_handler * handler) :
                _handler(handler), _interface_identifier(iid) {}
            
            /* fires when the ports_handler wants to transmit a transfer, 
            complemented by transfer_receive_callback */
            subject<transfer> transfer_transmit_event;
            
            /* this should subscribe to transfer_receive_event of the layer below this,
            complemented by transfer_transmit_event */
            void transfer_receive_callback(transfer t)
            {
                _handler->transfer_receive_callback(_interface_identifier, std::move(t));
            }

            interface_identifier get_interface_identifier() const {return _interface_identifier;}

            private:
            ports_handler * _handler;
            interface_identifier _interface_identifier;
        };

        private:

        std::list<service_endpoint> _services;
        std::list<interface_endpoint> _interfaces;

        auto _find_service(port_type port)
        {
            return std::find_if(_services.begin(), _services.end(), [&](const auto & pw){
                return pw.get_port() == port;
            });
        }

        auto _find_interface(interface_identifier iid)
        {
            return std::find_if(_interfaces.begin(), _interfaces.end(), [&](const auto & pw){
                return pw.get_interface_identifier() == iid;
            });
        }

        void transfer_receive_callback(interface_identifier iid, transfer t)
        {
            if (t.data_size() >= sizeof(Header))
            {
                Header h = parsers::byte_copy<Header>(t.data_begin());
                if (h.is_valid())
                {
                    auto pw = _find_service(h.destination);
                    /* just ignore ports that are not registered */
                    if (pw != _services.end())
                    {
                        /* hide the header and forward the transfer to the registered service */
                        t.remove_first_n(sizeof(Header));
                        pw->transfer_receive_event.emit(std::move(packet(std::move(t))));
                    }
                }
            }
        }
        
        public:

        /* this should subscribe to transfer_ack_event of the layer below this */
        void transfer_ack_callback(transfer_metadata tm)
        {

        }

        void transfer_transmit(port_type source, packet p)
        {
            Header h(p.destination_port(), source);
            p.push_front(to_bytes(h));

            auto i = _find_interface(p.interface_id());
            if (i != _interfaces.end())
                i->transfer_transmit_event.emit(std::move(p.to_transfer()));
        }

        /* use this to register a new interface, bind events and callbacks within the
        returned interface_endpoint object to a transfer factory */
        interface_endpoint & register_port(interface_identifier iid)
        {
            if (_find_interface(iid) != _interfaces.end())
                throw already_registered();

            return _interfaces.emplace_back(iid, this);
        }

        /* use this to register a new service, you must subscribe to events of interest
        within the returned service_endpoint reference */
        service_endpoint & register_port(port_type p)
        {
            if (_find_service(p) != _services.end())
                throw already_registered();
            
            return _services.emplace_back(p, this);
        }
    };
}


#endif