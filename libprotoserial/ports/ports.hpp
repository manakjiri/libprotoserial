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

#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/ports/headers.hpp"

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
    class port_service
    {

    };

    struct already_registered : std::exception {
        const char * what () const throw () {return "already_registered";}
    };

    class ports_handler
    {
        public:
        
        using Header = headers::ports_8b;
        using port_type = typename Header::port_type;

        private:

        struct port_wrapper
        {
            port_wrapper(port_type p, port_service * s) :
                port(p), service(s) {}

            port_type port; 
            port_service * service;
        };

        std::list<port_wrapper> _registered;

        public:

        /* fires when the ports_handler wants to transmit a transfer, 
        complemented by transfer_receive_callback */
        subject<transfer> transfer_transmit_event;

        /* this should subscribe to transfer_receive_event of the layer below this,
        complemented by transfer_transmit_event */
        void transfer_receive_callback(transfer t)
        {
            if (t.data_size() >= sizeof(Header))
            {
                Header h = parsers::byte_copy<Header>(t.data_begin());
                if (h.is_valid())
                {
                    t.data_hide_front(sizeof(Header));
                    
                }
            }
        }

        /* this should subscribe to transfer_ack_event of the layer below this */
        void transfer_ack_callback(transfer_metadata tm)
        {

        }

        /* shortcut for event subscribe for the layer below */
        void bind_to(fragmentation_handler & l)
        {
            l.transfer_receive_event.subscribe(&ports_handler::transfer_receive_callback, this);
            l.transfer_ack_event.subscribe(&ports_handler::transfer_ack_callback, this);
            transfer_transmit_event.subscribe(fragmentation_handler::transmit, &l);
        }


        void register_port(port_type p, port_service * s)
        {
            auto i = std::find_if(_registered.begin(), _registered.end(), [&](const auto & pw){
                return pw.port == p;
            });
            if (i != _registered.end())
                throw already_registered();

            auto & pw = _registered.emplace_back(p, s);
        }

    };
}


#endif