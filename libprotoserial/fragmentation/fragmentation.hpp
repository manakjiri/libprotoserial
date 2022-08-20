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

#include "libprotoserial/interface/parsers.hpp"
#include "libprotoserial/interface.hpp"

#include "libprotoserial/clock.hpp"

#include "libprotoserial/fragmentation/headers.hpp"
#include "libprotoserial/fragmentation/transfer.hpp"

#include <memory>

#ifndef SP_NO_IOSTREAM
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
        using rate_type = uint;

        static constexpr clock::duration rate2duration(rate_type rate, size_t size)
        {
            /*                           s             =            B       /      B/s      */
            return std::chrono::nanoseconds{std::chrono::seconds{size}} / (std::chrono::nanoseconds{std::chrono::seconds{rate}} / std::chrono::nanoseconds{std::chrono::seconds{1}});
        }

        struct configuration
        {
            /* these are ballpark estimates of the allowable tx and rx rates in bytes per second */
            uint tx_rate, rx_rate;
            /* this is the initial peer transmit rate */
            uint peer_rate;

            /* thresholds of the rx buffer levels (interface::status.free_receive_buffer) in bytes
            these influence our_status, frb_poor is the threshold where the status returns rx_poor() == true,
            frb_critical is when it returns rx_critical() == true */
            uint frb_poor, frb_critical;
            
            /* denominater value of the transmit rate that gets applied when the peer's
            status evaluates to rx_poor(), it triples for rx_critical() */
            float tr_decrease;
            /* counteracts tr_divider by incrementing the transmit rate when the conditions are favorable */
            uint tr_increase;
            
            /* 1 means that a retransmit request is sent immediately when it is detected that a fragment
            of such size could have been received given the rx_rate, values > 1 will increase this holdoff
            and are suitable when there are many connections at once. 
            note that this is not the only precondition */
            uint retransmit_request_holdoff_multiplier;
            /* inactivity timeout is a mechanism of purging stalled incoming or outgoing transfers 
            its initial value is derived from the fragment size and peer transmit or receive rate 
            and this multiplier */
            uint inactivity_timeout_multiplier;
            /* minimum hold duration for completed incoming transfers, it is necessary to hold onto 
            these received transfers in order to detect spurious retransmits that may happen due to
            fragment delays and lost ACKs */
            clock::duration minimum_incoming_hold_time;

            /* this tries to set good default values */
            constexpr configuration(const interface & i, uint rate, size_type rx_buffer_size)
            {
                tx_rate = rx_rate = rate;
                peer_rate = tx_rate / 5;
                retransmit_request_holdoff_multiplier = 3;

                //max_fragment_data_size = i.max_data_size();
                
                inactivity_timeout_multiplier = 5;
                minimum_incoming_hold_time = rate2duration(peer_rate, rx_buffer_size);
                tr_decrease = 2;
                //tr_increase = max_fragment_data_size / 10;

                //frb_poor = max_fragment_data_size + (fragment_overhead_size * 2);
                frb_critical = frb_poor * 3;
            }
        };

        fragmentation_handler(interface * i, configuration config, prealloc_size prealloc) :
            _config(std::move(config)), _interface(i) {}


        virtual void receive_callback(fragment p) = 0;
        virtual void main_task() = 0;
        virtual void transmit(transfer t) = 0;

        /* shortcut for event subscribe */
        void bind_to(interface & l)
        {
            l.receive_event.subscribe(&fragmentation_handler::receive_callback, this);
            l.transmit_began_event.subscribe(&fragmentation_handler::transmit_began_callback, this);
            transmit_event.subscribe(&interface::transmit, &l);
        }

        /* fires when the handler wants to transmit a fragment, complemented by receive_callback */
        subject<fragment> transmit_event;
        /* fires when the handler receives and fully reconstructs a fragment, complemented by transmit */
        subject<transfer> transfer_receive_event;
        /* fires when ACK was received from destination for this transfer */
        subject<transfer_metadata> transfer_ack_event;

        protected:

        virtual void transmit_began_callback(object_id_type id) {}

        configuration _config;
        prealloc_size _prealloc;
        interface * _interface;
    };
}


#endif
