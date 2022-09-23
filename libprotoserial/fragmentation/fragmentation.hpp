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


#ifndef _SP_FRAGMENTATION_HANDLER
#define _SP_FRAGMENTATION_HANDLER

#include "libprotoserial/interface.hpp"
#include "libprotoserial/interface/parsers.hpp"

#include "libprotoserial/clock.hpp"

#include "libprotoserial/fragmentation/headers.hpp"
#include "libprotoserial/fragmentation/transfer.hpp"

#include <memory>

#ifdef SP_ENABLE_IOSTREAM
#include <iostream>
/* it is hard to debug something that happens every 100 transfers using 
 * debugger alone, these enable different levels of debug prints */
//#define SP_FRAGMENTATION_DEBUG
//#define SP_FRAGMENTATION_WARNING

#ifdef SP_FRAGMENTATION_DEBUG
#define SP_FRAGMENTATION_WARNING
#endif
#endif


namespace sp
{
    //TODO move somewhere
    template<typename T>
    static constexpr void limit(const T & min, T & val, const T & max)
    {
        if (val > max) val = max;
        else if (val < min) val = min;
    }


    /* this is the base class for all fragmentation handlers */
    class fragmentation_handler
    {
        public:
        using index_type = transfer::index_type;
        using id_type = transfer::id_type;
        using size_type = transfer::data_type::size_type;
        using address_type = transfer::address_type;

        enum class transmit_status
        {
            DONE,
            /* no ACK came during the transmission */
            UNREACHABLE,
            /* the transfer was dropped during the transmit process,
            this usually happens due to interface overload */
            TIMEDOUT,
            /* part of the transfer was dropped by the interface,
            this usually means that the transfer was malformed */
            DROPPED
        };

        fragmentation_handler(interface & i, prealloc_size prealloc) :
            _prealloc(prealloc), _interface(i) {}

        void receive_callback(fragment f)
        {
            do_receive(std::move(f));
        }
        
        void main_task()
        {
            do_main();
        }

        void transmit(transfer t)
        {
            do_transmit(std::move(t));
        }
        
        /* shortcut for event subscribe */
        void bind_to(interface & l)
        {
            l.receive_event.subscribe(&fragmentation_handler::receive_callback, this);
            l.transmit_began_event.subscribe(&fragmentation_handler::transmit_began_callback, this);
            transmit_event.subscribe(&interface::transmit, &l);
        }

        interface_identifier interface_id() const
        {
            return _interface.interface_id();
        }

        /* fires when the handler wants to transmit a fragment, complemented by receive_callback */
        subject<fragment> transmit_event;
        /* fires when the handler receives and fully reconstructs a fragment, complemented by transmit */
        subject<transfer> transfer_receive_event;
        /* fires when the transmit process ended and the transfer was discarded by the handler */
        subject<object_id_type, transmit_status> transmit_complete_event;

        protected:

        virtual void do_receive(fragment f) = 0;
        virtual void do_main() = 0;
        virtual void do_transmit(transfer t) = 0;
        virtual void transmit_began_callback(object_id_type id) 
        {
            (void)id; //TODO
        }

        prealloc_size _prealloc;
        interface & _interface;
    };
}


#endif
