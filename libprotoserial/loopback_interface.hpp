
#include "libprotoserial/interface.hpp"

#include "etl/crc32.h"

namespace sp
{
    class loopback : public interface
    {
        public:
        typedef std::uint8_t                address_type;
        typedef std::uint8_t                size_type;
        typedef etl::crc32                  hash_algorithm;
        typedef hash_algorithm::value_type  hash_type;
        
        typedef std::function<byte(byte)>   transfer_function;

        struct __attribute__ ((__packed__)) header
        {
            address_type destination = 0;
            address_type source = 0;
            size_type size = 0;

            header() = default;
            header(const packet & p):
                destination(p.destination()), source(p.source()), size(p.data()->size()) {}
        };

        struct __attribute__ ((__packed__)) footer
        {
            hash_type hash = 0;

            footer() = default;
            footer(const bytes & b) :
                hash(hash_algorithm(reinterpret_cast<const uint8_t*>(b.begin()), 
                    reinterpret_cast<const uint8_t*>(b.end())).value()) {}
        };

        /* usi the wire to implement data in transit currupting function */
        loopback(uint id, address_type address, uint max_queue_size, uint max_packet_size, transfer_function wire = [](byte b){return b;}):
            interface("lo" + std::to_string(id), address, max_queue_size), _wire(wire), _max_packet_size(max_packet_size) {}

        void enable_isr() noexcept {_isr_enabled = true;}
        void disable_isr() noexcept {_isr_enabled = false;}
        bytes::size_type max_rx_packet_size() const noexcept {return _max_packet_size;}
        bool can_transmit() noexcept {return true;}
        
        packet parse_packet(bytes && buff) const 
        {
            bytes b = buff;
            //std::cout << "parse_packet got: " << buff << std::endl;
            /* copy the header into the header struct */
            header h;
            std::copy(b.begin(), b.begin() + sizeof(h), reinterpret_cast<byte*>(&h));
            /* copy the footer, shrink the container by the footer size and compute the checksum */
            footer f_parsed;
            std::copy(b.end() - sizeof(footer), b.end(), reinterpret_cast<byte*>(&f_parsed));
            b.shrink(0, sizeof(footer));
            footer f_computed(b);
            if (f_parsed.hash != f_computed.hash) throw bad_checksum();
            /* shrink the container by the header and return the packet object */
            b.shrink(sizeof(h), 0);
            return packet(interface::address_type(h.source), interface::address_type(h.destination), 
                std::move(b), static_cast<const interface*>(this));
        }
        
        bytes serialize_packet(packet && p) const 
        {
            auto b = to_bytes(header(p));
            b.push_back(p.data());
            auto f = to_bytes(footer(b));
            b.push_back(f);
            std::cout << "serialize_packet returning: " << b << std::endl;
            return b;
        }

        void transmit(bytes && buff) noexcept 
        {
            //std::cout << "transmit got: " << buff << std::endl;
            for (auto i = buff.begin(); i < buff.end(); i++)
                rx_isr(_wire(*i));
        }
        
        void rx_isr(byte b)
        {
            if (_isr_enabled)
            {
                /* push back the new byte */
                _rx_buffer->push_back(b);
                if (_rxed_packet_size == 0)
                {
                    /* try to get the packet size once we have enough of the header */
                    if (_rx_buffer->size() >= sizeof(header))
                    {
                        _rxed_packet_size = reinterpret_cast<header*>(_rx_buffer->data())->size 
                            + sizeof(header) + sizeof(footer);
                        /* someting's wrong //TODO handle better - rx_error perhaps? */
                        if (_rxed_packet_size > max_rx_packet_size())
                        {
                            _rxed_packet_size = 0;
                            rx_done();
                        }
                    }
                } 
                else if (_rxed_packet_size == _rx_buffer->size())
                {
                    /* once we have received the expected amount of bytes reset everything
                    so it is ready for next RX and call the rx_done() */
                    _rxed_packet_size = 0;
                    rx_done();
                }
            }
        }

        private:
        transfer_function _wire;
        size_type _rxed_packet_size = 0;
        uint _max_packet_size = 0;
        bool _isr_enabled = false;
    };

} // namespace sp