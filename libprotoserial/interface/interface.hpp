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


#ifndef _SP_INTERFACE_INTERFACE
#define _SP_INTERFACE_INTERFACE

#include "libprotoserial/utils/observer.hpp"
#include "libprotoserial/interface/fragment.hpp"
#include "libprotoserial/data/prealloc_size.hpp"

#include <string>
#include <queue>

namespace sp
{
    /* things left as implementation details for subclasses
     * - RX ISR and RX buffer (not the fragment queue)
     * - address (must be representable by interface::address), broadcast address
     * - error checking and data encoding
     */
    class interface
    {
        struct serialized
        {
            bytes data;
            object_id_type id;
        };

        public:

        using address_type = fragment::address_type;

        /* - name should uniquely identify the interface on this device
         * - address is the interface address, when a fragment is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of fragments the transmit queue can hold
         */
        interface(interface_identifier iid, address_type address, address_type broadcast_address, uint max_queue_size) : 
            _bytes_txed(0), _bytes_rxed(0), _max_queue_size(max_queue_size), _interface_id(iid), _address(address), 
            _broadcast_address(broadcast_address) {}

        virtual ~interface() {}
        
        void main_task() noexcept
        {
            //TODO some reflection of the load on the interface should be done here and in the put_received function
            /* here we have complete information about the interface - we can count the outgoing and incoming
            bytes, track the frequency and various other factors. we could probably also figure out how often and 
            how reliably this main_task function is called and gauge the system load */
            
            do_receive();

            /* if there is something in the queue, transmit it */
            if (!_tx_queue.empty() && can_transmit())
            {
                auto & serialized = _tx_queue.front();
                /* increment the TXed counter */
                _bytes_txed += serialized.data.size();
                /* if the transmit fails, the data is lost, since we've moved it out
                this should never happen since the specification states that can_transmit must be honored */
                if (do_transmit(std::move(serialized.data)))
                {
                    /* fire the transmit_began_event as a confirmation to the upper layers */
                    transmit_began_event.emit(serialized.id);
                }
                _tx_queue.pop();
            }
        }

        /* fills the source address and puts the fragment into the transmit queue 
        provided that the queue is not already full and p.data().size() is within [1, max_data_size()] */
        void transmit(fragment p)
        {
            /* sanity checks */
            if (is_writable() && p.destination() != 0 && p.data().size() <= max_data_size() && !p.data().is_empty())
            {
                /* complete the fragment */
                p.complete(get_address(), interface_id());
                auto id = p.object_id();
                _tx_queue.emplace(std::move(serialize_fragment(std::move(p))), id);
            }
        }

        bool is_writable() const {return _tx_queue.size() <= _max_queue_size;}
        uint writable_count() const {return _max_queue_size - _tx_queue.size();}
        
        interface_identifier interface_id() const noexcept {return _interface_id;}
        address_type get_address() const noexcept {return _address;}
        address_type get_broadcast_address() const noexcept {return _broadcast_address;}

        /* counter of the total number of received bytes over the lifetime of the interface */
        uint64_t get_received_count() const noexcept {return _bytes_rxed;}
        /* counter of the total number of transmitted bytes over the lifetime of the interface */
        uint64_t get_transmitted_count() const noexcept {return _bytes_txed;}
        
        /* returns the maximum size of the data portion in a fragment, this is interface dependent */
        virtual bytes::size_type max_data_size() const noexcept = 0;
        /* returns the prealloc size that, is used for the provided fragments to be transmitted, will
        enable copy-free push_back, push_front of the interface's footer and header */
        virtual prealloc_size minimum_prealloc() const noexcept = 0;

        /* emitted by the main_task function when a new fragment is received where the destination address matches
        the interface address */
        subject<fragment> receive_event;
        /* emitted by the main_task function when a new fragment is received where the destination address matches
        the interface broadcast address */
        subject<fragment> broadcast_receive_event;
        /* emitted by the main_task function when a new fragment is received where the destination address 
        does not match the interface address */
        subject<fragment> other_receive_event;
        /* this is an event that fires when the first byte of a fragment with this object id gets transmitted */
        subject<object_id_type> transmit_began_event;

        protected:

        /* TX (serialize_fragment => can_transmit => do_transmit) */
        /* fragment serialization is implemented here, exceptions can be thrown //FIXME no exceptions, move to optional
        the serialized fragment well be passed to transmit after can_transmit() returns true */
        virtual bytes serialize_fragment(fragment && p) const = 0;
        /* return true when the interface is ready to transmit */
        virtual bool can_transmit() noexcept = 0;
        /* transmit is implemented here, called from the main_task after can_transmit() returns true, 
        if the transmit fails for whatever reason, the transmit() function can return false and the 
        transmit will be reattempted with the same fragment later */
        virtual bool do_transmit(bytes && buff) noexcept = 0;
        
        /* RX (do_receive => put_received) */
        /* called from the main_task, this is where the derived class should handle fragment parsing, 
        returns the number of bytes to be processed at exit */
        virtual bytes::size_type do_receive() noexcept = 0;
        /* can be called from do_receive */
        void put_received(fragment && p) noexcept
        {
            if (p.destination() == _address)
                receive_event.emit(std::move(p));
            else if (p.destination() == _broadcast_address)
                broadcast_receive_event.emit(std::move(p));
            else
                other_receive_event.emit(std::move(p));
        }

        /* call this function every time some data is received,
        this is here for statistics and flow control */
        void log_received_count(bytes::size_type size)
        {
            _bytes_rxed += size;
        }

        private:

        std::queue<serialized> _tx_queue;
        uint64_t _bytes_txed, _bytes_rxed;
        uint _max_queue_size;
        interface_identifier _interface_id;
        address_type _address, _broadcast_address;
    };

}


#endif

