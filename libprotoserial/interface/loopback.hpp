
#ifndef _SP_INTERFACE_LOOPBACK
#define _SP_INTERFACE_LOOPBACK

#include "libprotoserial/interface/buffered.hpp"
#include "libprotoserial/interface/parsers.hpp"

namespace sp
{
    
    
    template<class header, class footer>
    class loopback_interface : public buffered_interface
    {
        public: 
        //typename typename header::size_type;

        typedef std::function<byte(byte)>   transfer_function;

        typedef sp::byte preamble_type;
        const preamble_type preamble = (preamble_type)0x55;
        const typename header::size_type preamble_length = 2;

        /* PACKET STRUCTURE: [preamble][preamble][header][data >= 1][footer] */


        /* use the wire to implement data in transit corrupting function */
        loopback_interface(uint id, interface::address_type address, uint max_queue_size, uint max_packet_size, uint buffer_size, transfer_function wire = [](byte b){return b;}):
            buffered_interface("lo" + std::to_string(id), address, max_queue_size, buffer_size), _wire(wire), _max_packet_size(max_packet_size) 
        {
            _read = get_rx_buffer();
            _write = get_rx_buffer();
        }

        bytes::size_type max_data_size() const noexcept {return _max_packet_size - sizeof(header) - sizeof(footer) - preamble_length;}
        bool can_transmit() noexcept {return true;}

        void do_receive() //FIXME modularize etc...
        {
            /* while we are trying to parse the buffer, the ISR is continually filling it
            (not in this case, obviously, but in the real world it will) 
            _write is the position of the last byte written, so we can read up to that point
            since this way we cannot possibly collide with the ISR 
            */
            auto read = _read;
            bool invalid_size = false;
            while (1)
            {
                //std::cout << "do_receive at: " << read._current - read._begin << " of: " << _write._current - _write._begin << std::endl;
                if (*read == preamble)
                {
                    auto start = read + 1;
                    /* we matched the preamble, now we would like to parse out the header
                    and check the size_check, since thats still pretty low effort,
                    we add 1 to the size because the preamble is not part of the header */
                    if ((typename header::size_type)distance(read, _write) >= (typename header::size_type)sizeof(header))
                    {
                        //TODO figure out a way to cache the parsed header when the below check does not pass
                        header h;
                        //std::copy(read + 1, read + (1 + sizeof(h)), reinterpret_cast<byte*>(&h));
                        //FIXME mess
                        auto it = start;
                        for (uint pos = 0; pos < sizeof(header); ++it, ++pos)
                            reinterpret_cast<byte*>(&h)[pos] = *it;

                        /* we got the header, if the simple size check passes, than we need to have at least 
                        that many bytes + the footer in the buffer */
                        if (h.is_size_valid())
                        {
                            typename header::size_type packet_size = h.size + sizeof(footer) + sizeof(header);
                            if((typename header::size_type)distance(read, _write) >= packet_size)
                            {
                                /* copy the entire packet so it can be passed into the parsing function */
                                //FIXME mess
                                bytes b(packet_size);
                                it = start;
                                for (uint pos = 0; pos < packet_size; ++it, ++pos)
                                    b[pos] = *it;

                                /* parse the packet */
                                try
                                {
                                    put_received(std::move(parsers::parse_packet<header, footer>(std::move(b), this)));
                                    /* parsing succeeded, move the read pointer */
                                    _read += packet_size + preamble_length;
                                }
                                catch(std::exception &e)
                                {
                                    /* parsing failed, move away from that invalid start */
                                    _read = start;
                                    std::cerr << "main exception: " << e.what() << '\n';
                                    //throw;
                                }
                            }
                        }
                        else
                        {
                            std::cout << "invalid size" << std::endl;
                            invalid_size = true;
                        }
                    }
                }
                if (_write == read)
                    break;
                else
                    ++read;
            }
            if (invalid_size && _write != _read)
                ++_read;
            std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
        }

        //bool is_received() const noexcept {return bool(_parsed);}
        //packet get_received() noexcept {return _parsed;}
        
        bytes serialize_packet(packet && p) const 
        {
            if (p.data().size() > max_data_size()) throw data_too_long();
            /* preallocate the container since we know the final size */
            auto b = bytes(0, 0, preamble_length + sizeof(header) + p.data().size() + sizeof(footer));
            /* preamble */
            auto pr = bytes(preamble_length);
            pr.set(preamble);
            b.push_back(pr);
            /* header */
            b.push_back(to_bytes(header(p)));
            /* data */
            b.push_back(p.data());
            /* footer */
            b.push_back(to_bytes(footer(b.begin() + preamble_length, b.end())));
            std::cout << "serialize_packet returning: " << b << std::endl;
            return b;
        }

        bool do_transmit(bytes && buff) noexcept 
        {
            //std::cout << "transmit got: " << buff << std::endl;
            for (auto i = buff.begin(); i < buff.end(); i++)
                rx_isr(_wire(*i));
            return true;
        }
        
        void rx_isr(byte b)
        {
            /* the iterator wraps around to the beginning at the end of the buffer */
            *(++_write) = b;
            _last_write = clock::now();
        }

        private:
        transfer_function _wire;
        buffered_interface::circular_iterator _write, _read;
        //packet _parsed;
        clock::time_point _last_write;
        typename header::size_type _rxed_packet_size = 0;
        uint _max_packet_size = 0;
    };

} // namespace sp
#endif
