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
 * abstracts away the physical interface, for that it needs to handle
 * - encoding, error detection and correction
 * 
 * interfaces are inherently interrupt driven, we need to either ensure
 * that callbacks don't take a long time or we resort to buffering, which 
 * seems like the safest and most versatile bet. But now we need to determine 
 * when the actual procesing takes place.
 * 
 * it seems that the right balance would be: decode, check, correct and 
 * determine whether the fragment belongs to us within the interrupt routine - 
 * that way only the relevant stuff gets put into the procesing queue.
 * 
 * in terms of structure it would be nice for the interface to only handle
 * the the bits and not have to deal with the more abstract things, like 
 * addresing -> let's introduce another component (address class) that deals 
 * with that in order to maintain modularity.
 * 
 * USB
 * - one interrupt callback for receive, fragments are already pieced together
 * - somewhat error checked
 * - 64B fragment size limit
 * 
 * UART
 * - need to receive byte-by-byte in an interrupt (we could make it more 
 *   efficient if we could constraint the fragment size to something like
 *   8B multiples so that we benefit from the hardware)
 * - don't know the size in advance, it should be constrained though
 * 
 * it would be nice to somehow ignore fragments that are not meant for us, 
 * which can be done if the address class exposes how long the address 
 * should be and then vets tha fragment. But we cannot necessarily stop 
 * listening as soon as we figure this out, because we need to catch the 
 * end of the fragment + it may be useful to have a callback with fragments 
 * that were not meant to us, but someone may be interrested for debuging
 * purposes for example.
 * 
 * so interface must already be concerned with addresses as well as with
 * all the lower layer stuff. However it should not be handling fragment 
 * fragmentation, I'd leave that to the thing that processes the RX queue.
 * 
 * which leads us to what the interface class should expose
 * - an RX event
 * - a bad RX event
 * - a TX complete event
 * 
 * interface has an RX and a TX queue, it will throw an exception? when the
 * TX fragment is over the maximum length. Events are fired from the main 
 * interface task, not from the interrupts, because event callbacks could 
 * take a long time to return. Queues isolate the interrupts from the main 
 * task. 
 * 
 * We can build the fragment fragmentation logic on top of the interface, 
 * this logic should have its own internal buffer for fragments received
 * from events, because once the event firess, the fragment is forgotten on 
 * the interface side to avoid the need for direct access to the interface's 
 * RX queue.
 * 
 * the TX queue should be size limited and an interface should be exposed
 * for checking whether we can push to the queue without being blocked by
 * the function call because the queue is too large.
 * 
 * so it should also expose
 * - write(fragment[, callback?]) a potentially blocking call
 * - writable() or something similar
 * 
 * 
 * Let's think again... Some interfaces are, as we said, easier to handle,
 * like USB. Can these simple ones be a subset of the harder ones, like UART?
 * 
 * For something like a uart it may be best to have some contiguous static 
 * buffer that we use as a ring buffer - here the isr is really simple, it 
 * just puts each byte it receives into the buffer and increments an index
 * and handles the index wrap. 
 * 
 * It may be best to introduce a start sequence into the fragments, something
 * like 0x5555 or whatever, which we use to lookup the start of the fragment
 * in this large array sort of efficiently. How to handle the wrap?
 * 
 * 
 */

#ifndef _SP_INTERFACE_INTERFACE
#define _SP_INTERFACE_INTERFACE

#include "libprotoserial/observer.hpp"
#include "libprotoserial/interface/fragment.hpp"

#include <string>
#include <queue>

namespace sp
{
    /* things left as implementation details for subclasses
     * - RX ISR and RX buffer (not the fragment queue)
     * - address (must be representable by interface::address), broadcast address
     * - error checking and data encoding
     * - filling the _name variable in the constructor
     */
    class interface
    {
        public:
        using address_type = fragment::address_type;

        struct status
        {
            status() = default;
            status(interface * i) : 
                available_transmit_slots(i->writable_count()) {}

            uint available_transmit_slots = 0;
        };

        struct bad_data : std::exception {
            const char * what () const throw () {return "bad_data";}
        };
        struct no_destination : std::exception {
            const char * what () const throw () {return "no_destination";}
        };
        struct not_writable : std::exception {
            const char * what () const throw () {return "not_writable";}
        };
        
        /* - name should uniquely identify the interface on this device
         * - address is the interface address, when a fragment is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of fragments the transmit queue can hold
         */
        interface(interface_identifier iid, address_type address, address_type broadcast_address, uint max_queue_size) : 
            _max_queue_size(max_queue_size), _interface_id(iid), _address(address), _broadcast_address(broadcast_address) {}

        virtual ~interface() {}
        
        void main_task() noexcept
        {
            /* if there is something in the queue, transmit it */
            if (!_tx_queue.empty() && can_transmit())
            {
                if (do_transmit(std::move(_tx_queue.front())))
                    _tx_queue.pop();
            }
            /* receive, this will call the put_received() function if a fragment is received */
            do_receive();
            emit_status();
        }

        /* fills the source address field, serializes the provided fragment object,
        and puts the serialized buffer into the TX queue. This wraps the write function, 
        if the write fails for whatever reason, the fragment is dropped */
        void write_noexcept(fragment p) noexcept
        {
            //TODO consider avoiding exceptions entirely here
            try {write(std::move(p));}
            catch(std::exception & e) {write_failed(e);}
        }

        virtual void write_failed(std::exception & e) {}

        /* fills the source address field,
        serializes the provided fragment object (which will throw if the fragment is malformed)
        and puts the serialized buffer into the TX queue */
        void write(fragment p)
        {
            /* sanity checks */
            if (!is_writable()) throw not_writable();
            if (p.destination() == 0) throw no_destination();
            if (p.data().size() > max_data_size() || p.data().is_empty()) throw bad_data();
            /* complete the fragment */
            p._complete(get_address(), interface_id());
            auto b = serialize_fragment(std::move(p));
            _tx_queue.push(std::move(b));
            emit_status();
        }

        //TODO is there a better way?
        bool is_writable() const {return _tx_queue.size() <= _max_queue_size;}
        uint writable_count() const {return _max_queue_size - _tx_queue.size();}
        
        interface_identifier interface_id() const noexcept {return _interface_id;}
        void reset_address(address_type addr) noexcept {_address = addr;}
        address_type get_address() const noexcept {return _address;}
        /* returns the maximum size of the data portion in a fragment, this is interface dependent */
        virtual bytes::size_type max_data_size() const noexcept = 0;

        /* emitted by the main_task function when a new fragment is received where the destination address matches
        the interface address */
        subject<fragment> receive_event;
        /* emitted by the main_task function when a new fragment is received where the destination address matches
        the interface broadcast address */
        subject<fragment> broadcast_receive_event;
        /* emitted by the main_task function when a new fragment is received where the destination address 
        does not match the interface address */
        subject<fragment> other_receive_event;

        subject<status> status_event;

        protected:

        /* TX (serialize_fragment => can_transmit => do_transmit) */
        /* fragment serialization is implemented here, exceptions can be thrown 
        the serialized fragment well be passed to transmit after can_transmit() returns true */
        virtual bytes serialize_fragment(fragment && p) const = 0;
        /* return true when the interface is ready to transmit */
        virtual bool can_transmit() noexcept = 0;
        /* transmit is implemented here, called from the main_task after can_transmit() returns true, 
        if the transmit fails for whatever reason, the transmit() function can return false and the 
        transmit will be reattempted with the same fragment later */
        virtual bool do_transmit(bytes && buff) noexcept = 0;
        
        /* RX (do_receive => put_received) */
        /* called from the main_task, this is where the derived class should handle fragment parsing */
        virtual void do_receive() noexcept = 0;
        void put_received(fragment && p) noexcept
        {
            if (p.destination() == _address)
                receive_event.emit(std::move(p));
            else if (p.destination() == _broadcast_address)
                broadcast_receive_event.emit(std::move(p));
            else
                other_receive_event.emit(std::move(p));
        }

        private:

        void emit_status()
        {
            status_event.emit(status(this));
        }

        /* queue of serialized fragments ready to be transmitted */
        std::queue<bytes> _tx_queue;
        uint _max_queue_size;

        interface_identifier _interface_id;
        address_type _address, _broadcast_address;
    };

}


#endif

