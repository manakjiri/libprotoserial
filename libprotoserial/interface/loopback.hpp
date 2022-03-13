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

#ifndef _SP_INTERFACE_LOOPBACK
#define _SP_INTERFACE_LOOPBACK

#include "libprotoserial/interface/buffered.hpp"
#include "libprotoserial/interface/parsers.hpp"

#ifndef SP_NO_IOSTREAM
//#define SP_LOOPBACK_DEBUG
//#define SP_LOOPBACK_WARNING
#endif

#ifdef SP_LOOPBACK_DEBUG
#define SP_LOOPBACK_WARNING
#endif

namespace sp
{
    namespace detail
    {
        template<class Header, class Footer>
        class loopback_interface : public buffered_interface
        {
            public: 

            typedef std::function<byte(byte)>   transfer_function;

            typedef sp::byte preamble_type;
            const preamble_type preamble = (preamble_type)0x55;
            const typename Header::size_type preamble_length = 2;

            /* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */


            /* use the wire to implement data in transit corrupting function */
            loopback_interface(uint id, interface::address_type address, uint max_queue_size, uint max_packet_size, uint buffer_size, transfer_function wire = [](byte b){return b;}):
                buffered_interface("lo" + std::to_string(id), address, max_queue_size, buffer_size), _wire(wire), _max_packet_size(max_packet_size) 
            {
                _read = get_rx_buffer();
                _write = get_rx_buffer();
            }

            bytes::size_type max_data_size() const noexcept {return _max_packet_size - sizeof(Header) - sizeof(Footer) - preamble_length;}
            bool can_transmit() noexcept {return true;}
            void write_failed(std::exception & e) 
            {
#ifdef SP_LOOPBACK_WARNING
                std::cout << "write_failed: " << e.what() << std::endl;
#endif
            }

            void do_receive() noexcept
            {
                /* while we are trying to parse the buffer, the ISR is continually filling it
                (not in this case, obviously, but in the real world it will) 
                _write is the position of the last byte written, so we can read up to that point
                since this way we cannot possibly collide with the ISR */
                auto read = _read;
                auto write = _write;

                /* while is necessary since we would never move forward in case we find a valid preamble but fail 
                before the parsing */
                while (read != write)
                {
#ifdef SP_LOOPBACK_DEBUG
                    std::cout << "do_receive before find: " << read._current - read._begin << " of " << write._current - write._begin << std::endl;
#endif
                    /* if this returns false than read == write and we break out of the while loop */
                    if (parsers::find(read, write, preamble))
                    {
                        /* read now points to the position of the preamble, we are no longer concerned with the preamble */
                        auto packet_start = read + 1;
                        /* check if the Header is already loaded into to buffer, if not this function will just return 
                        and try new time around, we cannot move the original _read just yet because of this, 
                        adding 1 to distance since we can also read the write byte */
#ifdef SP_LOOPBACK_DEBUG
                        std::cout << "do_receive after find: " << packet_start._current - packet_start._begin << " of " << write._current - write._begin << " value: " << (int)*packet_start << std::endl;
#endif
                        if ((size_t)distance(packet_start, write) + 1 >= sizeof(Header))
                        {
                            /* copy the Header into the Header structure byte by byte */
                            Header h = parsers::byte_copy<Header>(packet_start, packet_start + sizeof(Header));
                            if (h.is_valid(max_data_size()))
                            {
                                /* total packet size */
                                size_t packet_size = h.size + sizeof(Footer) + sizeof(Header);
                                /* once again, check that there are enough bytes in the buffer, this can still fail */
                                if ((size_t)distance(packet_start, write) + 1 >= packet_size)
                                {
                                    /* we have received the entire packet, prepare it for parsing */
                                    //bytes b(packet_size);
                                    //std::copy(packet_start, packet_start + packet_size, b.begin());
                                    auto b = parsers::byte_copy(packet_start, packet_start + packet_size);
                                    try
                                    {
                                        /* attempt the parsing */
#ifdef SP_LOOPBACK_DEBUG
                                        std::cout << "do_receive parse_packet gets: " << b << std::endl;
#endif
                                        put_received(std::move(parsers::parse_packet<Header, Footer>(std::move(b), this)));
                                        /* parsing succeeded, finally move the read pointer, we do not include the
                                        preamble length here because we don't necessarily know how long it was originally */
                                        _read = read + packet_size;
#ifdef SP_LOOPBACK_DEBUG
                                        std::cout << "do_receive after parse" << std::endl;
#endif
                                    }
                                    catch(std::exception &e)
                                    {
                                        /* parsing failed, move by one because there is no need to try and parse this again */
                                        _read = read + 1;
#ifdef SP_LOOPBACK_WARNING
                                        std::cerr << "do_receive parse exception: " << e.what() << '\n';
#endif
                                    }
#ifdef SP_LOOPBACK_DEBUG
                                    std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                                    return;
                                }
                                else
                                {
                                    /* once again we just failed the distance check for the whole packet this time, 
                                    we cannot save the read position because the Header check could be wrong as well */
                                    _read = read;
#ifdef SP_LOOPBACK_WARNING
                                    std::cout << "do_receive distance packet" << std::endl;
                                    std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                                    return;
                                }
                            }
                            else
                            {
                                /* we failed the size valid check, so this is either a corrupted Header or it's not a Header
                                at all, move the read pointer past this and try again */
                                _read = read = packet_start;
#ifdef SP_LOOPBACK_WARNING
                                std::cout << "do_receive invalid size: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                            }
                        }
                        else
                        {
                            /* we failed the distance check for the Header, we need to wait for the buffer to fill some more,
                            store the current read position as the global _read so that find returns faster once we come back */
                            _read = read;
#ifdef SP_LOOPBACK_WARNING
                            std::cout << "do_receive distance Header" << std::endl;
                            std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                            return;
                        }
                    }
                }
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
            }
            
            bytes serialize_packet(packet && p) const 
            {
            	//TODO we can avoid the copy here by calling reserve and hope that the container has enough capacity
                /* preallocate the container since we know the final size */
                auto b = bytes(0, 0, preamble_length + sizeof(Header) + p.data().size() + sizeof(Footer));
                /* preamble */
                auto pr = bytes(preamble_length);
                pr.set(preamble);
                b.push_back(pr);
                /* Header */
                b.push_back(to_bytes(Header(p)));
                /* data */
                b.push_back(p.data());
                /* swap the dst and the src address, this obviously 
                only works with the 8b8b Header */
                auto tmp = b[2];
                b[2] = b[3];
                b[3] = tmp;
                /* Footer */
                b.push_back(to_bytes(Footer(b.begin() + preamble_length, b.end())));
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "serialize_packet returning: " << b << std::endl;
#endif
                return b;
            }

            bool do_transmit(bytes && buff) noexcept 
            {
#ifdef SP_LOOPBACK_DEBUG
                std::cout << "transmit: " << buff << std::endl;
#endif
                for (auto i = buff.begin(); i < buff.end(); i++)
                    rx_isr(_wire(*i));
                return true;
            }
            
            void rx_isr(byte b)
            {
                /* the iterator wraps around to the beginning at the end of the buffer */
                *(++_write) = b;
            }

            private:
            transfer_function _wire;
            buffered_interface::circular_iterator _write, _read;
            uint _max_packet_size = 0;
        };
    }
} // namespace sp

#endif
