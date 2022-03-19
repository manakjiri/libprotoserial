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

#ifndef _SP_INTERFACE_BUFFEREDPARSED
#define _SP_INTERFACE_BUFFEREDPARSED

#include "libprotoserial/interface/buffered.hpp"
#include "libprotoserial/interface/parsers.hpp"

#ifndef SP_NO_IOSTREAM
//#define SP_BUFFERED_DEBUG
//#define SP_BUFFERED_WARNING
#endif

#ifdef SP_BUFFERED_DEBUG
#define SP_BUFFERED_WARNING
#endif

namespace sp
{
    namespace detail
    {
        template<class Header, class Footer>
        class buffered_parsed_interface : public buffered_interface
        {
            public:

            using preamble_type = byte;
            const preamble_type preamble = (preamble_type)0x55;
            const typename Header::size_type preamble_length = 2;

            buffered_parsed_interface(interface_identifier iid, address_type address, uint max_queue_size, uint buffer_size, 
                uint max_fragment_size):
                    buffered_interface(iid, address, max_queue_size, buffer_size), _max_fragment_size(max_fragment_size)
            {
                _read = get_rx_buffer();
                _write = get_rx_buffer();
            }

            bytes::size_type max_data_size() const noexcept {return _max_fragment_size - sizeof(Header) - sizeof(Footer) - preamble_length;}
            
            protected:

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
#ifdef SP_BUFFERED_DEBUG
                    std::cout << "do_receive before find: " << read._current - read._begin << " of " << write._current - write._begin << std::endl;
#endif
                    /* if this returns false than read == write and we break out of the while loop */
                    if (parsers::find(read, write, preamble))
                    {
                        /* read now points to the position of the preamble, we are no longer concerned with the preamble */
                        auto fragment_start = read + 1;
                        /* check if the Header is already loaded into to buffer, if not this function will just return 
                        and try new time around, we cannot move the original _read just yet because of this, 
                        adding 1 to distance since we can also read the write byte */
#ifdef SP_BUFFERED_DEBUG
                        std::cout << "do_receive after find: " << fragment_start._current - fragment_start._begin << " of " << write._current - write._begin << " value: " << (int)*fragment_start << std::endl;
#endif
                        if ((size_t)distance(fragment_start, write) + 1 >= sizeof(Header))
                        {
                            /* copy the Header into the Header structure byte by byte */
                            Header h = parsers::byte_copy<Header>(fragment_start, fragment_start + sizeof(Header));
                            if (h.is_valid(max_data_size()))
                            {
                                /* total fragment size */
                                size_t fragment_size = h.size + sizeof(Footer) + sizeof(Header);
                                /* once again, check that there are enough bytes in the buffer, this can still fail */
                                if ((size_t)distance(fragment_start, write) + 1 >= fragment_size)
                                {
                                    /* we have received the entire fragment, prepare it for parsing */
                                    //bytes b(fragment_size);
                                    //std::copy(fragment_start, fragment_start + fragment_size, b.begin());
                                    auto b = parsers::byte_copy(fragment_start, fragment_start + fragment_size);
                                    try
                                    {
                                        /* attempt the parsing */
#ifdef SP_BUFFERED_DEBUG
                                        std::cout << "do_receive parse_fragment gets: " << b << std::endl;
#endif
                                        put_received(std::move(parsers::parse_fragment<Header, Footer>(std::move(b), *this)));
                                        /* parsing succeeded, finally move the read pointer, we do not include the
                                        preamble length here because we don't necessarily know how long it was originally */
                                        _read = read + fragment_size;
#ifdef SP_BUFFERED_DEBUG
                                        std::cout << "do_receive after parse" << std::endl;
#endif
                                    }
                                    catch(std::exception &e)
                                    {
                                        /* parsing failed, move by one because there is no need to try and parse this again */
                                        _read = read + 1;
#ifdef SP_BUFFERED_WARNING
                                        std::cout << "do_receive parse exception: " << e.what() << '\n';
#endif
                                    }
#ifdef SP_BUFFERED_DEBUG
                                    std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                                    return;
                                }
                                else
                                {
                                    /* once again we just failed the distance check for the whole fragment this time, 
                                    we cannot save the read position because the Header check could be wrong as well */
                                    _read = read;
#ifdef SP_BUFFERED_WARNING
                                    std::cout << "do_receive distance fragment" << std::endl;
                                    std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                                    return;
                                }
                            }
                            else
                            {
                                /* we failed the size valid check, so this is either a corrupted Header or it's not a Header
                                at all, move the read pointer past this and try again */
                                _read = read = fragment_start;
#ifdef SP_BUFFERED_WARNING
                                std::cout << "do_receive invalid size: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                            }
                        }
                        else
                        {
                            /* we failed the distance check for the Header, we need to wait for the buffer to fill some more,
                            store the current read position as the global _read so that find returns faster once we come back */
                            _read = read;
#ifdef SP_BUFFERED_WARNING
                            std::cout << "do_receive distance Header" << std::endl;
                            std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
                            return;
                        }
                    }
                }
#ifdef SP_BUFFERED_DEBUG
                std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << _write._current - _write._begin << std::endl;
#endif
            }

            bytes serialize_fragment(fragment && p) const 
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
                /* Footer */
                b.push_back(to_bytes(Footer(b.begin() + preamble_length, b.end())));
#ifdef SP_BUFFERED_DEBUG
                std::cout << "serialize_fragment returning: " << b << std::endl;
#endif
                return b;
            }

            buffered_interface::circular_iterator _write, _read;
            uint _max_fragment_size;
        };
    }
} // namespace sp


#endif

