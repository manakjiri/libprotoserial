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
    class service
    {

    };

    template<class Header>
    class ports_handler
    {

        public:
        
        using port_type = typename Header::port_type;

        /* fires when the ports_handler wants to transmit a transfer, 
        complemented by transfer_receive_callback */
        subject<transfer> transfer_transmit_event;

        /* this should subscribe to transfer_receive_event of the layer below this,
        complemented by transfer_transmit_event */
        void transfer_receive_callback(transfer t)
        {

        }


        void register_port( service * s)
        {

        }

    };
}


#endif