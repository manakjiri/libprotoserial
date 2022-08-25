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


#ifndef _SP_FRAGMENTATION_MINIMAL
#define _SP_FRAGMENTATION_MINIMAL

#include "libprotoserial/fragmentation/fragmentation.hpp"
#include "libprotoserial/fragmentation/transfer_handler.hpp"

namespace sp::detail
{
    template<typename Header>
    class base_fragmentation_handler : public fragmentation_handler
    {
        public:
        using message_types = typename Header::message_types;

        struct configuration
        {
            /* these are ballpark estimates of the allowable tx and rx rates in bytes per second */
            bit_rate tx_rate, rx_rate;
            /* this is the initial peer transmit rate */
            bit_rate peer_rate;

            /* thresholds of the rx buffer levels (interface::status.free_receive_buffer) in bytes
            these influence our_status, frb_poor is the threshold where the status returns rx_poor() == true,
            frb_critical is when it returns rx_critical() == true */
            uint frb_poor, frb_critical;
            
            /* denominator value of the transmit rate that gets applied when the peer's
            status evaluates to rx_poor(), it triples for rx_critical() */
            double tr_decrease;
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
            constexpr configuration(const interface & i, bit_rate rate, size_type rx_buffer_size)
            {
                tx_rate = rx_rate = rate;
                peer_rate = tx_rate / 5;
                retransmit_request_holdoff_multiplier = 3;

                //max_fragment_data_size = i.max_data_size();
                
                inactivity_timeout_multiplier = 5;
                minimum_incoming_hold_time = peer_rate.bit_period() * rx_buffer_size;
                tr_decrease = 2;
                //tr_increase = max_fragment_data_size / 10;

                //frb_poor = max_fragment_data_size + (fragment_overhead_size * 2);
                frb_critical = frb_poor * 3;
            }
        };

        protected:

        class peer_state
        {
            public:
            peer_state(address_type a, const configuration & c) : //TODO
                addr(a), tx_rate(c.peer_rate), last_rx(never()), tx_holdoff(clock::now()) {}

            address_type addr;
            /* from our point of view */
            uint tx_rate;
            /* from our point of view */
            clock::time_point last_rx, tx_holdoff;

            bool in_transmit_holdoff() const {return tx_holdoff > clock::now();}
        };

        auto find_peer(address_type addr)
        {
            auto peer = std::find_if(_peer_states.begin(), _peer_states.end(), [&](const peer_state & ps){
                return ps.addr == addr;
            });
            if (peer == _peer_states.end())
            {
                _peer_states.emplace_front(addr, _config);
                return _peer_states.begin();
            }
            else
                return peer;
        }

        inline auto find_outgoing(std::function<bool(const transfer_handler<Header> &)> pred)
        {
            return std::find_if(_outgoing_transfers.begin(), _outgoing_transfers.end(), pred);
        }

        inline auto find_incoming(std::function<bool(const transfer_handler<Header> &)> pred)
        {
            return std::find_if(_incoming_transfers.begin(), _incoming_transfers.end(), pred);
        }

        inline Header make_header(message_types type, index_type fragment_pos, const transfer_handler<Header> & t) const
        {
            return Header(type, fragment_pos, t.fragments_count(), t.get_id(), t.get_prev_id(), 0);
        }
        inline Header make_header(message_types type, const Header & h) const
        {
            return Header(type, h.fragment(), h.fragments_total(), h.get_id(), h.get_prev_id(), 0);
        }

        /* data size before the header is added */
        inline size_type max_fragment_data_size() const
        {
            /* _interface->max_data_size() is the maximum size of a fragment's data */
            return _interface->max_data_size() - sizeof(Header);
        }

        /* implementation of fragmentation_handler::receive_callback */
        /* the callback handles the incoming fragments, it does not handle any timeouts, sending requests, 
        or anything that assumes periodicity, the main_task is for that */
        void receive_callback(fragment f)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "receive_callback got: " << f << std::endl;
#endif
            if (f && f.data().size() >= sizeof(Header))
            {
                /* copy the header from the fragment data after some obvious sanity checks */
                auto h = parsers::byte_copy<Header>(f.data().begin());
                if (h.is_valid())
                {
                    /* discard the header from fragment's data */
                    f.data().shrink(sizeof(Header), 0);
                    handle_fragment(h, std::move(f));
                }
            }
        }
        
        /* implementation of fragmentation_handler::main_task */
        void main_task()
        {

        }
        
        /* implementation of fragmentation_handler::transmit */
        void transmit(transfer t)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "transmit got: " << t << std::endl;
#elif defined(SP_FRAGMENTATION_WARNING)
            std::cout << "transmit got id " << (int)t.get_id() << std::endl;
#endif
            _outgoing_transfers.emplace_back(std::move(t), max_fragment_data_size());
        }

        /* implementation of fragmentation_handler::transmit_began_callback */
        void transmit_began_callback(object_id_type id)
        {
            auto pt = find_outgoing([id](const transfer_handler<Header> & tr){
                return tr.object_id() == id;
            });
            if (pt != _outgoing_transfers.end())
            {
                
            }
        }

        public:

        void print_debug() const
        {
#ifndef SP_NO_IOSTREAM
            std::cout << "incoming_transfers: " << _incoming_transfers.size() << std::endl;
            for (const auto & t : _incoming_transfers)
                std::cout << static_cast<transfer>(t) << std::endl;
            
            std::cout << "outgoing_transfers: " << _outgoing_transfers.size() << std::endl;
            for (const auto & t : _outgoing_transfers)
                std::cout << static_cast<transfer>(t) << std::endl;
#endif
        }

        private:

        configuration _config;
        std::list<peer_state> _peer_states;
        std::list<transfer_handler<Header>> _incoming_transfers, _outgoing_transfers;
    };
}

#endif
