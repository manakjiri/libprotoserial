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
 * > We can build the fragment fragmentation logic on top of the interface, 
 * > this logic should have its own internal buffer for fragments received
 * > from events, because once the event fires, the fragment is forgotten on 
 * > the interface side to avoid the need for direct access to the interface's 
 * > RX queue.
 * 
 * fragmentation handler needs to implement basic congestion and flow control.
 * each handler manages only one interface, each interface can be connected to
 * multiple different interfaces with different addresses, so it would be beneficial
 * to broadcast the state information to share it with everybody, but in terms
 * of total overhead this may not be the best solution (since we are sending
 * data that aims to prevent congestion...)
 * 
 * in order for this to be useful, the information needs to be up-to-date,
 * so perhaps the most logical solution would be to embed it into the data
 * itself. Perhaps a single additional byte in the fragmentation header
 * might be sufficient to convey enough information about the receiver's state
 * 
 * handler should store the following information about peers
 * - current holdoff times and capacity estimates
 * 
 * the status value should reflect the receiver's available capacity either 
 * in absolute terms (which I do not find appealing) or using increase/decrease
 * signaling - there will be other factors that collectively influence the 
 * rate of transmition. Reason why I think absolute would not work is because 
 * these systems are not necessarily linear.
 * 
 * 
 * 
 * 
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
/* it is hard to debug someting that happens every 100 transfers using 
 * debugger alone, these enable different levels of debug prints */
//#define SP_FRAGMENTATION_DEBUG
//#define SP_FRAGMENTATION_WARNING
#ifdef SP_FRAGMENTATION_DEBUG
#define SP_FRAGMENTATION_WARNING
#endif
#endif



namespace sp
{
    struct fragmentation_handler
    {
        using index_type = transfer::index_type;
        using id_type = transfer::id_type;
        using size_type = transfer::data_type::size_type;
        using address_type = transfer::address_type;
        using rate_type = uint;

        static clock::duration rate2duration(rate_type rate, size_t size)
        {
            /*                           s             =            B       /      B/s      */
            return std::chrono::nanoseconds{std::chrono::seconds{size}} / (std::chrono::nanoseconds{std::chrono::seconds{rate}} / std::chrono::nanoseconds{std::chrono::seconds{1}});
        }

        template<typename T>
        static constexpr void limit(const T & min, T & val, const T & max)
        {
            if (val > max) val = max;
            else if (val < min) val = min;
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
            configuration(const interface & i, uint rate, size_type rx_buffer_size)
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

        public:

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
        template<typename Header>
        struct transfer_handler : public transfer
        {
            /* receive constructor, f is the first fragment of the transfer (maximum fragment size is derived
            from this fragment's size)
            note that this cannot infer the actual size of the final transfer based on this fragment (unless this
            is the last one as well), so a worst-case scenario is assumed (fragments_total * max_fragment_size) 
            and the internal data() container will get resized in the put_fragment function when the last fragment 
            is received */
            transfer_handler(fragment && f, const Header & h) : 
                transfer(transfer_metadata(f.source(), f.destination(), f.interface_id(), f.timestamp_creation(), h.get_id(), 
                h.get_prev_id()), std::move(f.data())), max_fragment_size(f.data().size()), fragments_total(h.fragments_total())
            {
                /* reserve space for up to fragments_total fragments. There is no need to regard prealloc_size since this 
                is the receive constructor. fragments_total is always >= 1, so this works for all cases, expand does nothing 
                for arguments (0, 0) */
                data().expand(0, (fragments_total - 1) * max_fragment_size);
            }

            /* transmit constructor, max_fragment_size is the maximum fragment data size excluding the fragmentation header */
            transfer_handler(transfer && t, size_type max_fragment_size) : 
                transfer(std::move(t)), max_fragment_size(max_fragment_size), fragments_total(0)
            {
                auto size = data().size();
                /* calculate the fragments_total count correctly, ie. assume max = 4, then
                for size = 2 -> total = 1, size = 4 -> total = 1 but size = 5 -> total = 2 */
                fragments_total = size / max_fragment_size + (size % max_fragment_size == 0 ? 0 : 1);
            }

            /* returns the fragment's data size, this does not include the Header,
            use only when constructed using the transmit constructor */
            size_type fragment_size(index_type pos)
            {
                pos -= 1;
                if (pos >= fragments_total)
                    return 0;
                
                auto start = pos * max_fragment_size;
                auto end = std::min(start + max_fragment_size, data().size());
                return end - start;
            }

            /* returns a fragment containing the correct metadata with data copied from pos of the transfer,
            the data does not contain handler's header! it does however have enough prealloc for it plus
            the requested prealloc. Just use fragment.data().push_front(header_bytes) to add the header */
            fragment get_fragment(index_type pos, const prealloc_size & alloc)
            {
                auto data_size = fragment_size(pos);
                if (data_size == 0)
                    return fragment();
                
                bytes data = alloc.create(sizeof(Header), data_size, 0);
                auto start = fragment_start(pos);

                std::copy(start, start + data_size, data.begin());
                return fragment(std::move(get_fragment_metadata()), std::move(data));
            }

            /* this function assumes that this was created using the receive constructor, it only 
            concernes itself with the fragment's data, not its metadata. */
            bool put_fragment(index_type pos, const fragment & f)
            {
                auto expected_max_size = fragment_size(pos);
                if (f.data().size() > expected_max_size)
                    return false;
            
                auto start = fragment_start(pos);
                std::copy(f.data().begin(), f.data().end(), start);

                /* resize the data() so it reflects the actual size now that we have received
                the last fragment, more on that in the receive constructor comment */
                if (pos == fragments_total)
                    data().shrink(0, expected_max_size - f.data().size());

                return true;
            }

            /* maximum fragment data size excluding the fragmentation header,
            max_fragment_size * fragments_total >= data().size() should always hold */
            size_type max_fragment_size;
            /* max_fragment_size * fragments_total >= data().size() should always hold */
            index_type fragments_total;

            protected:
            
            inline data_type::iterator fragment_start(index_type pos)
            {
                return data().begin() + ((pos - 1) * max_fragment_size);
            }
        };

        configuration _config;
        prealloc_size _prealloc;
        interface * _interface;
    };
}


#endif
