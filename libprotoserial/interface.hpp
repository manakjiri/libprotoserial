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
 * should be and then vets tha packet. But we cannot necesarly stop 
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
 * - writable() or something similiar
 */


#include "libprotoserial/container.hpp"
#include "libprotoserial/clock.hpp"
#include "libprotoserial/observer.hpp"

#include <string>
#include <queue>
#include <stdexcept>

namespace sp
{

    /* things left as an implementation details for subclasses
     * - RX ISR and RX buffer (not the packet queue)
     * - address (must be representable by interface::address) and the header
     * - error checking and data encoding
     * - filling the _name variable in the constructor
     */
    class interface
    {
        public:
        /* an integer that can hold any used device address, 
        the actual address format is interface specific */
        typedef uint    address_type;

        struct packet_too_short : std::exception {
            const char * what () const throw () {return "packet_too_short";}
        };

        struct packet_too_long : std::exception {
            const char * what () const throw () {return "packet_too_long";}
        };

        struct bad_checksum : std::exception {
            const char * what () const throw () {return "bad_checksum";}
        };

        /* interface packet representation */
        class packet
        {
            public:

            packet(address_type src, address_type dst, bytes && d, const sp::interface *i = nullptr) :
                _data(d), _timestamp(clock::now()), _interface(i), _source(src), _destination(dst) {}
            
            //packet():packet(0, 0, bytes(), nullptr) {}

            constexpr clock::time_point timestamp() const noexcept {return _timestamp;}
            constexpr const sp::interface* interface() const noexcept {return _interface;}
            constexpr address_type source() const noexcept {return _source;}
            constexpr address_type destination() const noexcept {return _destination;}
            //constexpr bytes* data() noexcept {return &_data;}
            constexpr const bytes* data() const noexcept {return &_data;}

            private:
            bytes _data;
            clock::time_point _timestamp;
            const sp::interface *_interface;
            address_type _source, _destination;
        };
        
        
        /* the RX ISR must be disabled when initializing, interface will enable it automatically
        after initialization */
        interface(std::string name, address_type address, uint max_queue_size) : 
            _max_queue_size(max_queue_size), _name(name), _address(address) {}

        
        void main_task()
        {
            /* initialize and enable the RX ISR at first */
            if (!_initialized)
            {
                _rx_ring_buffer.reserve(_max_queue_size);
                for (uint i = 0; i < _max_queue_size; i++)
                    _rx_ring_buffer.emplace_back(bytes(0, 0, max_rx_packet_size()));
                
                _rx_buffer = &_rx_ring_buffer.at(_current_rx_buff);
                _initialized = true;
                enable_isr();
            }
            /* if there is something in the queue, transmit it */
            if (!_tx_queue.empty() && can_transmit())
            {
                transmit(std::move(_tx_queue.front()));
                _tx_queue.pop();
            }
            /* parse all the received data, this will parse all unparsed containers and
            emit events for them */
            while(_to_parse_buff != _current_rx_buff)
            {
                auto buff = std::move(_rx_ring_buffer.at(_to_parse_buff));
                if (buff)
                {
                    auto p = parse_packet(std::move(buff));
                    _rx_ring_buffer.at(_to_parse_buff) = std::move(bytes(0, 0, max_rx_packet_size()));
                    if (p.destination() == _address)
                        packed_rxed_event.emit(p);
                    else
                        other_packed_rxed_event.emit(p);
                }
                increment_to_parse_buff();
            }
        }

        bool is_writable() {return _tx_queue.size() <= _max_queue_size;}
        std::string name() const {return _name;}

        void write(packet p)
        {
            while(!is_writable()) {main_task();} //?
            _tx_queue.push(std::move(serialize_packet(std::move(p))));
        }


        /* emitted by the main_task function when a new packet is received where the destination address matches
        the interface address */
        subject<packet> packed_rxed_event;
        /* emitted by the main_task function when a new packet is received where the destination address 
        does not match the interface address */
        subject<packet> other_packed_rxed_event;

        protected:

        /* reenebles the RX ISR */
        virtual void enable_isr() noexcept = 0;
        /* disables the RX ISR */
        virtual void disable_isr() noexcept = 0;
        /* returns the maximum needed size of the rx buffer */
        virtual bytes::size_type max_rx_packet_size() const noexcept = 0;
        /* called from the main_task() as soon as a new packet is received into the ring buffer, 
        this function should parse that raw buffer into an interface::packet instance */
        virtual packet parse_packet(bytes && buff) const = 0;
        /* packet serialization is implemented here, exceptions can be thrown 
        the serialized packet well be passed to transmit after can_transmit() returns true */
        virtual bytes serialize_packet(packet && p) const = 0;
        virtual bool can_transmit() noexcept = 0;
        /* transmit is implemented here, called from the main_task after can_transmit() */
        virtual void transmit(bytes && buff) noexcept = 0;

        /* call this from the RX ISR as soon as the _rx_buffer is ready to be processed,
        after this call a fresh _rx_buffer will be put in place ready for another packet */
        void rx_done()
        {
            disable_isr();
            increment_current_rx_buff();
            _rx_buffer = &_rx_ring_buffer.at(_current_rx_buff);
            enable_isr();
        }

        /* one of _rx_ring_buffer, it will be always initialized as bytes(0, 0, max_rx_packet_size()),
        so one needs to just call _rx_buffer->push_back(new_byte) for simple single-byte RX
        or _rx_buffer->expand(0, number_of_new_bytes); and copy the bytes into _rx_buffer for cases like USB RX*/
        bytes *_rx_buffer;

        private:

        inline void increment_current_rx_buff() {if (++_current_rx_buff > _rx_ring_buffer.size()) _current_rx_buff = 0;}
        inline void increment_to_parse_buff() {if (++_to_parse_buff > _rx_ring_buffer.size()) _to_parse_buff = 0;}

        /* a ring buffer of bytes objects */
        std::vector<bytes> _rx_ring_buffer;
        /* queue of serialized packets ready to be transmitted */
        std::queue<bytes> _tx_queue;
        uint _max_queue_size = 0;
        /* indexes in the _rx_ring_buffer, _current_rx_buff current buffer which is used for RX
        _to_parse_buff buffer which will be parsed next */
        uint _current_rx_buff = 0, _to_parse_buff = 0;
        /* name of the interface, usually something along the lines "uart2", "usb0" */
        std::string _name;
        address_type _address;
        bool _initialized = false;
    };
}



