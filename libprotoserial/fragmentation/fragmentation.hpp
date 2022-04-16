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
#endif


#ifdef SP_FRAGMENTATION_DEBUG
#define SP_FRAGMENTATION_WARNING
#endif

namespace sp
{
    class fragmentation_handler
    {
        public:

        using Header = headers::fragment_8b8b;
        using types = Header::message_types;
        using index_type = transfer::index_type;
        using id_type = transfer::id_type;
        using size_type = fragment::data_type::size_type;
        using address_type = fragment::address_type;

        static clock::duration rate2duration(uint rate, uint size)
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
            
            /* maximum size of a fragment's data (max_fragment_data_size >= sizeof(Header) + data.size()) */
            size_type max_fragment_data_size;
            /* approximately how many more bytes are added by lower layers */
            size_type fragment_overhead_size;
            
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
            configuration(const interface & i, uint rate, size_type added_fragment_overhead, size_type rx_buffer)
            {
                tx_rate = rx_rate = rate;
                peer_rate = tx_rate / 5;
                retransmit_request_holdoff_multiplier = 3;

                max_fragment_data_size = i.max_data_size();
                fragment_overhead_size = added_fragment_overhead;
                
                inactivity_timeout_multiplier = 5;
                minimum_incoming_hold_time = rate2duration(peer_rate, rx_buffer);
                tr_decrease = 2;
                tr_increase = max_fragment_data_size / 10;

                frb_poor = max_fragment_data_size + (fragment_overhead_size * 2);
                frb_critical = frb_poor * 3;
            }
        };

        private:

        struct transfer_wrapper : public transfer
        {
            transfer_wrapper(transfer && t, fragmentation_handler * handler) :
                transfer(std::move(t)), _handler(handler), timestamp_accessed(clock::now()), 
                retransmitions(0), transmit_index(1) {}

            /* returns the number of fragments needed to transmit this transfer,
            this obviously depends on the mode but we can make some assumptions:
            -   when in mode1 transfer has internally preallocated the needed number of 
                slots for fragments so this returns that number
            -   when in mode 2 slots are allocated on demand with new data, so this 
                returns the computed number of needed fragments based on the data size */
            index_type fragments_count() const 
            {
                if (is_complete())
                    /* mode 2 */
                    return data_size() / _handler->max_fragment_size() + 
                        (data_size() % _handler->max_fragment_size() == 0 ? 0 : 1);
                else
                    /* mode 1 */
                    return _data.size();
            }
            
            /* mode 1) only */
            bool is_complete() const
            {
                for (auto b = _begin(); b != _end(); ++b)
                {
                    if (b->is_empty())
                        return false;
                }
                return true;
            }

            bool has_data() const {return !_data.empty();}
            
            transfer move_out_data()
            {
                return transfer(transfer_metadata(*this), std::move(_data));
            }

            bool completely_transmitted() const
            {
                return transmit_index > fragments_count();
            }

            /* returns the next fragment including the Header in succession to be transmitted, 
            only use when completely_transmitted() == false */
            fragment get_next_fragment()
            {
                auto ret = get_fragment(transmit_index);
                bytes h = to_bytes(_handler->make_header(types::FRAGMENT, transmit_index, *this));
                ret.data().push_front(std::move(h));
                transmit_done();
                return ret;
            }

            /* mode 1) only */
            index_type missing_fragment() const
            {
                for (std::size_t i = 0; i < _data.size(); ++i)
                {
                    if (_data.at(i).is_empty())
                        return (index_type)(i + 1);
                }
                return 0;
            }
            
            /* returns the requested fragment at pos, it is assumed that
            this fragment was requested by other peer and this is therefore a retransmit,
            use missing_fragment() to obtain pos */
            fragment get_retransmit_fragment(index_type pos)
            {
                auto ret = get_fragment(pos);
                bytes h = to_bytes(_handler->make_header(types::FRAGMENT, pos, *this));
                ret.data().push_front(std::move(h));
                retransmit_done();
                return ret;
            }

            uint retransmitions_count() const {return retransmitions;}
            clock::time_point last_accessed() const {return timestamp_accessed;}

            private:
            fragmentation_handler * _handler;
            clock::time_point timestamp_accessed;
            uint retransmitions, transmit_index;

            /* mode 2) only, preferably */
            fragment get_fragment(index_type fragment_pos) const
            {
                if (fragment_pos == 0) throw std::invalid_argument("fragment_pos == 0");
                fragment_pos -= 1;
                auto p_size = _handler->max_fragment_size();
                if (fragment_pos * p_size > data_size()) throw std::invalid_argument("fragment_pos * p_size > data_size()");

                bytes data(sizeof(Header), 0, p_size);
                auto b = data_begin() + fragment_pos * p_size, e = data_end();
                for (; b != e && data.size() < p_size; ++b) data.push_back(*b);
                
                return fragment(destination(), std::move(data));
            }
            /* fragment get_fragment(index_type fragment_pos) const
            {
                auto p_size = get_fragment_size(fragment_pos);
                bytes data(sizeof(Header), 0, p_size); //TODO add overalloc for interface header
                
                auto b = data_begin() + fragment_pos * p_size;
                for (; data.size() < p_size; ++b) data.push_back(*b);
                
                return fragment(destination(), std::move(data));
            }

            data_type::size_type get_fragment_size(index_type fragment_pos) const
            {
                if (fragment_pos == 0) throw std::invalid_argument("fragment_pos == 0");
                fragment_pos -= 1;

                auto p_size = _handler->max_fragment_size();
                if (fragment_pos * p_size > data_size()) throw std::invalid_argument("fragment_pos * p_size > data_size()");
                
                auto from_end = distance(data_begin() + fragment_pos * p_size, data_end());
                return from_end < p_size ? from_end : p_size;
            } */

            void transmit_done()
            {
                timestamp_accessed = clock::now();
                if (!completely_transmitted())
                    ++transmit_index;
            }
            void retransmit_done() 
            {
                timestamp_accessed = clock::now();
                ++retransmitions;
            }
        };

        
        struct status
        {
            using type = typename Header::status_type;
            type value;
            
            status(const Header & h) : value(h.status()) {}
            status(const interface::status & s, const configuration & c)
            {
                value = 0;
                if (s.receive_buffer_level >= c.frb_critical)
                    value |= 0x03;
                else if (s.receive_buffer_level >= c.frb_poor)
                    value |= 0x01;
            }
            
            bool rx_poor() {return (value & 0x01) == 0x01;}
            bool rx_critical() {return (value & 0x03) == 0x03;}
            
        };

        struct peer_state
        {
            peer_state(address_type a, const configuration & c)  :
                addr(a), tx_rate(c.peer_rate), last_rx(never()), tx_holdoff(clock::now()) {}

            address_type addr;
            /* from our point of view */
            uint tx_rate;
            /* from our point of view */
            clock::time_point last_rx, tx_holdoff;

            bool in_transmit_holdoff() const {return tx_holdoff > clock::now();}
        };

        auto peer_state_find(address_type addr)
        {
            auto peer = std::find_if(_peer_states.begin(), _peer_states.end(), [&](peer_state & ps){
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

        void peer_state_update(peer_state & peer, const Header & h)
        {
            /* update from the status field */
            auto s = status(h);
            if (s.rx_critical())
            {
                peer.tx_rate /= _config.tr_decrease * 3;
#ifdef SP_FRAGMENTATION_WARNING
                std::cout << "received status critical from " << (int)peer.addr << std::endl;
#endif
            }
            else if (s.rx_poor())
            {
                peer.tx_rate /= _config.tr_decrease;
#ifdef SP_FRAGMENTATION_WARNING
                std::cout << "received status poor from " << (int)peer.addr << std::endl;
#endif
            }
            else
            {
                peer.tx_rate += _config.tr_increase;
#ifdef SP_FRAGMENTATION_DEBUG
                std::cout << "increasing tx_rate for " << (int)peer.addr << std::endl;
#endif
            }
            limit(_config.peer_rate / 2, peer.tx_rate, _config.tx_rate * 2);
            
            peer.last_rx = clock::now();
        }

        /* if the transfer was not accessed within this duration, than it should be safe to drop such tranfer */
        clock::duration peer_state_inactivity_timeout(peer_state & peer, size_type data_size)
        {
            return rate2duration(peer.tx_rate, data_size) * _config.inactivity_timeout_multiplier;
        }
        /* if the transfer was not accessed within this duration, than it should be safe to drop such tranfer */
        clock::duration peer_state_inactivity_timeout(address_type addr, size_type data_size)
        {
            return peer_state_inactivity_timeout(*peer_state_find(addr), data_size);
        }
        
        /* returns true when it is a good time to transmit to this peer, this assumes that 
        the fragment is transmitted if this evaluates to true,
        data_size is assumed to be just the fragment data size, additional overhead gets accounted for */
        bool peer_state_can_transmit(peer_state & peer, size_type data_size)
        {
            if (peer.in_transmit_holdoff())
                return false;
            else
            {
                peer.tx_holdoff = clock::now() + rate2duration(peer.tx_rate, final_fragment_size(data_size));
                //std::cout << "setting tx_holdoff " << (peer.tx_holdoff - clock::now()).count() << " ns from now " << d.count() << std::endl;
                return true;
            }
        }
        /* returns true when it is a good time to transmit to this peer, this assumes that 
        the fragment is transmitted if this evaluates to true,
        data_size is assumed to be just the fragment data size, fragment_overhead_size gets accounted for */
        bool peer_state_can_transmit(address_type addr, size_type data_size)
        {
            return peer_state_can_transmit(*peer_state_find(addr), data_size);
        }
        
        bool peer_state_can_reqretransmit(peer_state & peer, const transfer_wrapper & tr)
        {
            if (older_than(tr.timestamp_modified(), _config.retransmit_request_holdoff_multiplier *
                rate2duration(_config.rx_rate, _config.max_fragment_data_size))) //FIXME correct size
            {
                if (peer_state_can_transmit(peer, _config.max_fragment_data_size)) //HACK
                {
                    return true;
                }
            }
            return false;
        }
        bool peer_state_can_reqretransmit(address_type addr, const transfer_wrapper & tr)
        {
            return peer_state_can_reqretransmit(*peer_state_find(addr), tr);
        }

        public:

        fragmentation_handler(interface_identifier iid, configuration config) :
            _config(std::move(config)), _interface_identifier(iid) {}

        fragmentation_handler(const interface & i, configuration config) :
            fragmentation_handler(i.interface_id(), std::move(config)) {}


        /* the callback handles the incoming fragments, it does not handle any timeouts, sending requests, 
        or anything that assumes periodicity, the main_task is for that */
        void receive_callback(fragment p) noexcept
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "receive_callback got: " << p << std::endl;
#endif
            if (p && p.data().size() >= sizeof(Header))
            {
                /* copy the header from the fragment data after some obvious sanity checks, discard 
                the header from fragment's data afterward */
                auto h = parsers::byte_copy<Header>(p.data().begin());
                if (h.is_valid())
                {
                    p.data().shrink(sizeof(Header), 0);
                    handle_fragment(h, std::move(p));
                }
            }
        }

        void main_task()
        {
            auto it = _incoming_transfers.begin();
            while (it != _incoming_transfers.end())
            {
                if (!it->has_data())
                {
                    /* we have the transfer_progress object but it does not own any transfer, which means
                    that the block below must have been executed in the near past
                    drop the transfer_progress after some sufficiently long period since its last access */
                    if (older_than(it->last_accessed(), _config.minimum_incoming_hold_time)) //TODO add a check for _incoming_transfers size 
                        it = _incoming_transfers.erase(it);
                }
                /* checks whether any of the transfers is complete, if so emit it as receive event */
                else if (it->is_complete())
                {
                    /* this does not check the can_transmit() since the check is conservative 
                    and queing ACKs would result in a lot of added complexity */
                    transmit_event.emit(std::move(
                        fragment(it->source(), 
                        to_bytes(make_header(types::FRAGMENT_ACK, it->fragments_count(), *it)))
                    ));
                    transfer_receive_event.emit(std::move(it->move_out_data()));
                    /* moved the local copy of the transfer out of the handler, but we
                    will hold onto that transfer_metadata structure for a while longer - in case
                    the ACK fragment gets lost, the source will try to retransmit this transfer
                    thinking that we didn't notice it */
                }
                else
                {
                    auto peer = peer_state_find(it->source());
                    if (older_than(it->timestamp_modified(), peer_state_inactivity_timeout(*peer, it->data_size())))
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "timed out incoming: " << *it << std::endl;
#endif
                        /* drop the incoming transfer because it is inactive for too long */
                        it = _incoming_transfers.erase(it);
                    }
                    else if (can_transmit() && peer_state_can_reqretransmit(*peer, *it))
                    {
                        /* find the missing fragment's index and request a retransmit */
                        auto index = it->missing_fragment();
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "requesting retransmit for id " << (int)it->get_id() << " index " << (int)index << std::endl;
#endif
                        transmit_event.emit(std::move(
                            fragment(it->source(), 
                            to_bytes(make_header(types::FRAGMENT_REQ, index, *it)))
                        ));
                    }
                }
                /* check again because an erase can happen in this branch as well, 
                we don't need to worry about skipping a transfer when the erase happens
                since we will return into this function later anyway */
                if (it != _incoming_transfers.end())
                    ++it;
            }
            /* check for stale outgoing transfers, it may happen that the ACK didn't arrive, 
            it is not ACKed back, so that can happen */
            it = _outgoing_transfers.begin();
            while (it != _outgoing_transfers.end())
            {
                auto peer = peer_state_find(it->destination());
                if (older_than(it->last_accessed(), peer_state_inactivity_timeout(*peer, it->data_size())))
                {
                    /* drop the outgoing transfer because it is inactive for too long */
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "timed out outgoing id " << (int)it->get_id() << std::endl;
#endif
                    it = _outgoing_transfers.erase(it);
                }
                else if (can_transmit() && peer_state_can_transmit(*peer, _config.max_fragment_data_size)) //FIXME size
                {
                    if (!it->completely_transmitted())
                    {
                        /* this transfer was just put into the outgoing queue */
                        transmit_event.emit(std::move(it->get_next_fragment()));
                    }
                    else
                    {
                        /* either the destination is dead or the first fragment got lost during
                        transit, try to retransmit the first fragment */
#ifdef SP_FRAGMENTATION_WARNING
                            std::cout << "retransmitting first fragment of id " << (int)it->get_id() << std::endl;
#endif
                        transmit_event.emit(std::move(it->get_retransmit_fragment(1)));
                    }

                }
                else
                    ++it;
            }
        }

        void interface_status_callback(interface::status status)
        {
            _interface_status = status;
        }

        void transmit(transfer t)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "transmit got: " << t << std::endl;
#elif defined(SP_FRAGMENTATION_WARNING)
            std::cout << "transmit got id " << (int)t.get_id() << std::endl;
#endif
            _outgoing_transfers.emplace_back(std::move(t), this);
        }

        size_type max_fragment_size() const
        {
            return _config.max_fragment_data_size - sizeof(Header);
        }

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

        /* shortcut for event subscribe */
        void bind_to(interface & l)
        {
            l.receive_event.subscribe(&fragmentation_handler::receive_callback, this);
            l.status_event.subscribe(&fragmentation_handler::interface_status_callback, this);
            transmit_event.subscribe(&interface::write_noexcept, &l);
        }

        /* fires when the handler wants to transmit a fragment, complemented by receive_callback */
        subject<fragment> transmit_event;
        /* fires when the handler receives and fully reconstructs a fragment, complemented by transmit */
        subject<transfer> transfer_receive_event;
        /* fires when ACK was received from destination for this transfer */
        subject<transfer_metadata> transfer_ack_event;

        private:

        Header make_header(types type, index_type fragment_pos, const transfer_wrapper & tp)
        {
            return Header(type, fragment_pos, tp.fragments_count(), tp.get_id(), tp.get_prev_id(), our_status());
        }
        Header make_header(types type, const Header & h)
        {
            return Header(type, h.fragment(), h.fragments_total(), h.get_id(), h.get_prev_id(), our_status());
        }

        status::type our_status() {return status(_interface_status, _config).value;}
        bool can_transmit() const {return _interface_status.used_transmit_slots <= 1;}
        
        size_type final_fragment_size(size_type data_size) const 
        {
            return data_size + _config.fragment_overhead_size + sizeof(Header);
        }

        void handle_fragment(const Header & h, fragment && p)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "handle_fragment got: " << p << std::endl;
#endif
            /* we got a fragment from a peer, update the state */
            auto peer = peer_state_find(p.source());
            peer_state_update(*peer, h);

            /* handle the reception of user fragments and their ordering */
            if (h.type() == types::FRAGMENT)
            {
                /* branch for handling _incoming_transfers */
                /* check if we already know that incoming transfer ID */
                auto it = std::find_if(_incoming_transfers.begin(), _incoming_transfers.end(), 
                    [&](const auto & t){
                        //FIXME use tr->match() in both cases, as per spec
                        if (t.has_data())
                            return t.get_id() == h.get_id() && t.match(p);
                        else
                            return t.get_id() == h.get_id();
                });

                if (it == _incoming_transfers.end())
                {
#ifdef SP_FRAGMENTATION_DEBUG
                    std::cout << "creating new incoming transfer id " << (int)h.get_id() << std::endl;
#endif
                    /* we don't know this transfer ID, create new incoming transfer */
                    auto& t = _incoming_transfers.emplace_back(transfer(_interface_identifier, h), this);
                    t._assign(h.fragment(), std::move(p));
                }
                else
                {
                    /* we know this ID, now we need to check if we have already received this transfer and
                    is therefor duplicate, or whether we are in the process of receiving it */
                    if (it->has_data())
                    {
#ifdef SP_FRAGMENTATION_DEBUG
                        std::cout << "assigning to existing incoming transfer id " << (int)h.get_id() << " at " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        /* the ID is in known transfers, we need to add the incoming fragment to it */
                        it->_assign(h.fragment(), std::move(p));
                    }
                    else
                    {
                        /* we, for some reason, received a fragment of already received transfer, the ACK
                        from us probably got lost in transit, just reply with another ACK and ignore this fragment */
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "sending ACK for already received id " << (int)h.get_id() << std::endl;
#endif
                        transmit_event.emit(std::move(fragment(p.source(), 
                            std::move(to_bytes(make_header(types::FRAGMENT_ACK, h)))
                        )));
                    }
                }
            }
            else
            {
                /* branch for handling _outgoing_transfers */
                auto it = std::find_if(_outgoing_transfers.begin(), _outgoing_transfers.end(), 
                    [&](const auto & t){return t.get_id() == h.get_id() && t.match_as_response(p);});
                
                if (it != _outgoing_transfers.end())
                {
                    if (h.type() == types::FRAGMENT_REQ && can_transmit())
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "handling retransmit request of id " << (int)h.get_id() << " fragment " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        /* fullfill the retransmit request */
                        transmit_event.emit(std::move(it->get_retransmit_fragment(h.fragment())));
                    }
                    else if (h.type() == types::FRAGMENT_ACK)
                    {
#ifdef SP_FRAGMENTATION_DEBUG
                        std::cout << "got fragment ACK for id " << (int)h.get_id() << std::endl;
#endif
                        /* emit the ACK event for the sender and destroy this outgoing transfer
                        since we've done our job and don't need it anymore - in contrast to the 
                        incoming transfer where the transmitted ACK may not be received, here we
                        can be sure */
                        transfer_ack_event.emit(std::move(it->get_metadata()));
                        it = _outgoing_transfers.erase(it);
                    }
                }
            }
        }

        configuration _config;
        std::list<transfer_wrapper> _incoming_transfers, _outgoing_transfers;
        std::list<peer_state> _peer_states;
        interface_identifier _interface_identifier;
        interface::status _interface_status;
    };
}




#endif
