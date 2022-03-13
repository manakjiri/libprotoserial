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
 * determine whether the packet belongs to us within the interrupt routine - 
 * that way only the relevant stuff gets put into the procesing queue.
 * 
 * in terms of structure it would be nice for the interface to only handle
 * the the bits and not have to deal with the more abstract things, like 
 * addresing -> let's introduce another component (address class) that deals 
 * with that in order to maintain modularity.
 * 
 * USB
 * - one interrupt callback for receive, packets are already pieced together
 * - somewhat error checked
 * - 64B packet size limit
 * 
 * UART
 * - need to receive byte-by-byte in an interrupt (we could make it more 
 *   efficient if we could constraint the packet size to something like
 *   8B multiples so that we benefit from the hardware)
 * - don't know the size in advance, it should be constrained though
 * 
 * it would be nice to somehow ignore packets that are not meant for us, 
 * which can be done if the address class exposes how long the address 
 * should be and then vets tha packet. But we cannot necessarily stop 
 * listening as soon as we figure this out, because we need to catch the 
 * end of the packet + it may be useful to have a callback with packets 
 * that were not meant to us, but someone may be interrested for debuging
 * purposes for example.
 * 
 * so interface must already be concerned with addresses as well as with
 * all the lower layer stuff. However it should not be handling packet 
 * fragmentation, I'd leave that to the thing that processes the RX queue.
 * 
 * which leads us to what the interface class should expose
 * - an RX event
 * - a bad RX event
 * - a TX complete event
 * 
 * interface has an RX and a TX queue, it will throw an exception? when the
 * TX packet is over the maximum length. Events are fired from the main 
 * interface task, not from the interrupts, because event callbacks could 
 * take a long time to return. Queues isolate the interrupts from the main 
 * task. 
 * 
 * We can build the packet fragmentation logic on top of the interface, 
 * this logic should have its own internal buffer for packets received
 * from events, because once the event firess, the packet is forgotten on 
 * the interface side to avoid the need for direct access to the interface's 
 * RX queue.
 * 
 * the TX queue should be size limited and an interface should be exposed
 * for checking whether we can push to the queue without being blocked by
 * the function call because the queue is too large.
 * 
 * so it should also expose
 * - write(packet[, callback?]) a potentially blocking call
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
 * It may be best to introduce a start sequence into the packets, something
 * like 0x5555 or whatever, which we use to lookup the start of the packet
 * in this large array sort of efficiently. How to handle the wrap?
 * 
 * 
 */

#ifndef _SP_INTERFACE_INTERFACE
#define _SP_INTERFACE_INTERFACE

#include "libprotoserial/container.hpp"
#include "libprotoserial/clock.hpp"
#include "libprotoserial/observer.hpp"

#include <string>
#include <queue>

namespace sp
{

    /* things left as implementation details for subclasses
     * - RX ISR and RX buffer (not the packet queue)
     * - address (must be representable by interface::address), broadcast address //TODO
     * - error checking and data encoding
     * - filling the _name variable in the constructor
     */
    class interface
    {
        public:
        /* an integer that can hold any used device address, the actual address format 
        is interface specific, address 0 is reserved internally and should never appear 
        in a packet */
        typedef uint    address_type;

        struct bad_data : std::exception {
            const char * what () const throw () {return "bad_data";}
        };

        struct no_destination : std::exception {
            const char * what () const throw () {return "no_destination";}
        };

        struct not_writable : std::exception {
            const char * what () const throw () {return "not_writable";}
        };

        class packet_metadata
        {
            public:
            using address_type = interface::address_type;

            packet_metadata(address_type src, address_type dst, const interface *i, clock::time_point timestamp_creation):
                _timestamp_creation(timestamp_creation), _interface(i), _source(src), _destination(dst) {}

            constexpr clock::time_point timestamp_creation() const {return _timestamp_creation;}
            constexpr const interface* get_interface() const noexcept {return _interface;}
            constexpr address_type source() const noexcept {return _source;}
            constexpr address_type destination() const noexcept {return _destination;}

            void set_destination(address_type dst) {_destination = dst;}

            protected:
            clock::time_point _timestamp_creation;
            const sp::interface *_interface;
            address_type _source, _destination;
        };

        /* interface packet representation */
        class packet : public packet_metadata
        {
            public:

            typedef bytes   data_type;

            packet(address_type src, address_type dst, data_type && d, const sp::interface *i) :
                //_data(d), _timestamp_creation(clock::now()), _interface(i), _source(src), _destination(dst) {}
                packet_metadata(src, dst, i, clock::now()), _data(std::move(d)) {}

            /* this object can be passed to the interface::write() function */
            packet(address_type dst, data_type && d) :
                packet((address_type)0, dst, std::move(d), nullptr) {}

            packet():
                packet(0, data_type()) {}
            
            constexpr const data_type& data() const noexcept {return _data;}
            constexpr data_type& data() noexcept {return _data;}
            constexpr void _complete(address_type src, const sp::interface *i) {_source = src; _interface = i;}
            
            bool carries_information() const {return _data && _destination;}
            explicit operator bool() const {return carries_information();}

            protected:
            data_type _data;
        };
        
        /* - name should uniquely identify the interface on this device
         * - address is the interface address, when a packet is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of packets the transmit queue can hold
         */
        interface(std::string name, address_type address, uint max_queue_size) : 
            _max_queue_size(max_queue_size), _name(name), _address(address) {}
        
        void main_task() noexcept
        {
            /* if there is something in the queue, transmit it */
            if (!_tx_queue.empty() && can_transmit())
            {
                if (do_transmit(std::move(_tx_queue.front())))
                    _tx_queue.pop();
            }
            /* receive, this will call the put_received() function if a packet is received */
            do_receive();
        }

        /* fills the source address field, serializes the provided packet object,
        and puts the serialized buffer into the TX queue. This wraps the write function, 
        if the write fails for whatever reason, the packet is dropped */
        void write_noexcept(packet p) noexcept
        {
            //TODO consider avoiding exceptions entirely here
            try {write(std::move(p));}
            catch(std::exception & e) {write_failed(e);}
        }

        virtual void write_failed(std::exception & e) {}

        /* fills the source address field,
        serializes the provided packet object (which will throw if the packet is malformed)
        and puts the serialized buffer into the TX queue */
        void write(packet p)
        {
            /* sanity checks */
            if (!is_writable()) throw not_writable();
            if (p.destination() == 0) throw no_destination();
            if (p.data().size() > max_data_size() || p.data().is_empty()) throw bad_data();
            /* complete the packet */
            p._complete(get_address(), this);
            auto b = serialize_packet(std::move(p));
            _tx_queue.push(std::move(b));
        }

        //TODO is there a better way?
        bool is_writable() const {return _tx_queue.size() <= _max_queue_size;}
        
        /* returns a unique name of the interface */
        std::string name() const noexcept {return _name;}
        void reset_address(address_type addr) noexcept {_address = addr;}
        address_type get_address() const noexcept {return _address;}
        /* returns the maximum size of the data portion in a packet, this is interface dependent */
        virtual bytes::size_type max_data_size() const noexcept = 0;

        /* emitted by the main_task function when a new packet is received where the destination address matches
        the interface address */
        subject<packet> receive_event;
        /* emitted by the main_task function when a new packet is received where the destination address 
        does not match the interface address */
        subject<packet> other_receive_event;

        protected:

        /* TX (serialize_packet => can_transmit => do_transmit) */
        /* packet serialization is implemented here, exceptions can be thrown 
        the serialized packet well be passed to transmit after can_transmit() returns true */
        virtual bytes serialize_packet(packet && p) const = 0;
        /* return true when the interface is ready to transmit */
        virtual bool can_transmit() noexcept = 0;
        /* transmit is implemented here, called from the main_task after can_transmit() returns true, 
        if the transmit fails for whatever reason, the transmit() function can return false and the 
        transmit will be reattempted with the same packet later */
        virtual bool do_transmit(bytes && buff) noexcept = 0;
        
        /* RX (do_receive => put_received) */
        /* called from the main_task, this is where the derived class should handle packet parsing */
        virtual void do_receive() noexcept = 0;
        void put_received(packet && p) noexcept
        {
            if (p.destination() == _address)
                receive_event.emit(p);
            else
                other_receive_event.emit(p);
        }

        private:

        /* queue of serialized packets ready to be transmitted */
        std::queue<bytes> _tx_queue;
        uint _max_queue_size = 0;

        std::string _name;
        address_type _address = 0;
    };

}

bool operator==(const sp::interface::packet & lhs, const sp::interface::packet & rhs)
{
    return ((lhs.get_interface() && rhs.get_interface()) ? (lhs.get_interface()->name() == rhs.get_interface()->name()) : true) 
    && lhs.source() == rhs.source() && lhs.destination() == rhs.destination() && lhs.data() == rhs.data();
}

bool operator!=(const sp::interface::packet & lhs, const sp::interface::packet & rhs)
{
    return !(lhs == rhs);
}

#ifndef SP_NO_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::interface::packet& p) 
{
    os << "dst: " << p.destination() << ", src: " << p.source();
    os << ", int: " << (p.get_interface() ? p.get_interface()->name() : "null");
    os << ", " << p.data();
    return os;
}
#endif


#endif

