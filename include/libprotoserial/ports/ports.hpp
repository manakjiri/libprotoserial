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


#ifndef _SP_PORTS_PORTS
#define _SP_PORTS_PORTS

#include "libprotoserial/interface/interface.hpp"
#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/ports/headers.hpp"
#include "libprotoserial/ports/packet.hpp"

#include <list>
#include <algorithm>

#ifdef SP_ENABLE_EXCEPTIONS
#include <stdexcept>
#endif

#ifdef SP_ENABLE_IOSTREAM
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
#ifdef SP_ENABLE_EXCEPTIONS
    struct already_registered : std::exception {
        const char * what () const throw () {return "already_registered";}
    };
    struct invalid_port : std::exception {
        const char * what () const throw () {return "invalid_port";}
    };
#endif

    /* port handler does not make any assumptions as to what a service looks like, meaning there
     * is no base class, just the internal service_endpoint, which houses the events and callbacks */
    class ports_handler
    {
        public:
        
        using Header = headers::ports_8b;
        using port_type = packet::port_type;

        class service_endpoint
        {
            ports_handler & _handler;
            port_type _port; 

            public:
            service_endpoint(port_type p, ports_handler & handler):
                _handler(handler), _port(p) {}

            /* service should subscribe to this in order to receive transfers */
            subject<packet> receive_event;

            /* service should call this when it wants to transmit a packet, creating a subject<packet>
            within the service and subscribing using this fuction is recommended, there should never 
            be a need to call this directly outside of testing, so that is why it has an underscore  */
            void _transmit_callback(packet p)
            {
                _handler.transmit(_port, std::move(p));
            }

            auto get_port() const {return _port;}
        };

        class interface_endpoint
        {
            ports_handler & _handler;
            interface_identifier _interface_identifier;

            public:
            interface_endpoint(interface_identifier iid, ports_handler & handler) :
                _handler(handler), _interface_identifier(iid) {}
            
            /* fires when the ports_handler wants to transmit a transfer, 
            complemented by transfer_receive_callback */
            subject<transfer> transfer_transmit_event;
            
            /* this should subscribe to transfer_receive_event of the layer below this,
            complemented by transfer_transmit_event, there should never be a need to call 
            this directly outside of testing, so that is why it has an underscore */
            void _transfer_receive_callback(transfer t)
            {
                _handler.transfer_receive_callback(_interface_identifier, std::move(t));
            }

            interface_identifier get_interface_id() const {return _interface_identifier;}
        };

        private:

        std::list<service_endpoint> _services;
        std::list<interface_endpoint> _interfaces;
        prealloc_size _prealloc;

        auto _find_service(const port_type port) const
        {
            return std::find_if(_services.begin(), _services.end(), [&](const auto & pw){
                return pw.get_port() == port;
            });
        }

        auto _find_interface(const interface_identifier iid) const
        {
            return std::find_if(_interfaces.begin(), _interfaces.end(), [&](const auto & pw){
                return pw.get_interface_id() == iid;
            });
        }

        void transfer_receive_callback(const interface_identifier iid, transfer t)
        {
            if (t.data().size() >= sizeof(Header))
            {
                Header h = parsers::byte_copy<Header>(t.data().begin());
                if (h.is_valid())
                {
                    auto pw = _find_service(h.destination);
                    /* just ignore ports that are not registered */
                    if (pw != _services.end())
                    {
                        /* hide the header and forward the transfer to the registered service */
                        t.data().shrink(sizeof(Header), 0);
                        pw->receive_event.emit(packet(std::move(t), h));
                    }
                }
            }
        }
        
        public:

        ports_handler(const prealloc_size prealloc = prealloc_size()):
            _prealloc(std::move(prealloc)) {}

        /* this should subscribe to transfer_ack_event of the layer below this */
        void transfer_ack_callback(object_id_type id)
        {

        }

        void transmit(port_type source, packet p)
        {
            p.set_source_port(source);
            if (p.is_transmit_ready())
            {
                Header h(p.destination_port(), p.source_port());
                p.data().push_front(to_bytes(h));

                auto i = _find_interface(p.interface_id());
                if (i != _interfaces.end())
                {
                    i->transfer_transmit_event.emit(
                        transfer(p.get_transfer_metadata(), std::move(p.data()))
                    );
                }
            }
        }

        /* use this to register a new interface, bind events and callbacks within the
        returned interface_endpoint object to a fragmentation layer */
        [[nodiscard]] interface_endpoint & register_interface(interface_identifier iid)
        {
            if (_find_interface(iid) != _interfaces.end())
                SP_THROW_OR_TERMINATE(already_registered());
            
            return _interfaces.emplace_back(iid, *this);
        }

        /* shortcut for registering fragmentation_handler using register_interface properly */
        void register_interface(fragmentation_handler & l)
        {
            auto & ie = register_interface(l.interface_id());
            ie.transfer_transmit_event.subscribe(&fragmentation_handler::transmit, &l);
            l.transfer_receive_event.subscribe(&ports_handler::interface_endpoint::_transfer_receive_callback, &ie);
        }

        /* use this to register a new service, you must subscribe to events of interest
        within the returned service_endpoint reference */
        [[nodiscard]] service_endpoint & register_service(const port_type p)
        {
            if (p == packet::invalid_port)
                SP_THROW_OR_TERMINATE(invalid_port());

            if (_find_service(p) != _services.end())
                SP_THROW_OR_TERMINATE(already_registered());
            
            return _services.emplace_back(p, *this);
        }

        /* note: make sure that callback functions will not get called after unregistering */
        void unregister_service(const port_type p)
        {
            auto endpoint = _find_service(p);
            if (endpoint != _services.end())
            {
                _services.erase(endpoint);
            }
        }

        /* use this to properly over-allocate the data (bytes) containers that you pass into 
        this ports_handler, this will enable copy-free prepend of header for this layer */
        prealloc_size get_minimum_prealloc() const noexcept
        {
            return _prealloc + prealloc_size(sizeof(Header), 0);
        }

        /* returns a port number, from the free to use range, which is unoccupied */
        port_type get_free_port() const noexcept
        {
            port_type ret = 100; //TODO formalize

            //FIXME optimize
            while (_find_service(ret) != _services.end())
                ++ret;

            return ret;
        }
    };
}


#endif
