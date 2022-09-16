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

#ifndef _SP_INTERFACE_BUFFERED
#define _SP_INTERFACE_BUFFERED

#include <limits>

#include "libprotoserial/interface/interface.hpp"
#include "libprotoserial/interface/parsers.hpp"


#ifdef SP_ENABLE_IOSTREAM
//#define SP_BUFFERED_DEBUG
//#define SP_BUFFERED_WARNING
//#define SP_BUFFERED_CRITICAL
#endif

#ifdef SP_BUFFERED_DEBUG
#define SP_BUFFERED_WARNING
#endif
#ifdef SP_BUFFERED_WARNING
#define SP_BUFFERED_CRITICAL
#endif

namespace sp
{
    namespace detail
    {
        class buffered_interface : public interface
        {
            public:
            
        	/* - name should uniquely identify the interface on this device
			 * - address is the interface address, when a fragment is received where destination() == address
			 *   then the receive_event is emitted, otherwise the other_receive_event is emitted
			 * - max_queue_size sets the maximum number of fragments the transmit queue can hold
			 * - buffer_size sets the size of the receive buffer in bytes
			 */
            buffered_interface(interface_identifier iid, address_type address, address_type broadcast_address, uint max_queue_size, uint buffer_size):
                    interface(iid, address, broadcast_address, max_queue_size), _rx_buffer(buffer_size), _byte_count(0), _postpone_by_one(false)
            {
            	_write_it = _rx_buffer.begin();
            }


            /* iterator pointing into the _rx_buffer, it supports wrapping */
            struct circular_iterator 
            {
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = bytes::difference_type;
                using value_type        = bytes::value_type;
                using pointer           = bytes::pointer; 
                using reference         = bytes::reference;

                circular_iterator(bytes::iterator begin, bytes::iterator end, bytes::iterator start) : 
                    _begin(begin), _end(end), _current(start) {}

                circular_iterator(bytes & buff, bytes::iterator start) :
                    circular_iterator(buff.begin(), buff.end(), start) {}

                circular_iterator():
                    circular_iterator(nullptr, nullptr, nullptr) {}

                circular_iterator(const circular_iterator &) = default;
                circular_iterator(circular_iterator &&) = default;
                circular_iterator & operator=(const circular_iterator &) = default;
                circular_iterator & operator=(circular_iterator &&) = default;

                reference operator*() const { return *_current; }
                pointer operator->() { return _current; }

                // Prefix increment
                circular_iterator& operator++() 
                {
                    ++_current; 
                    if (_current >= _end) _current = _begin;
                    return *this;
                }

                circular_iterator& operator+=(uint shift)
                {
                    _current += shift;
                    if (_current >= _end) _current -= (_end - _begin);
                    return *this; 
                }

                friend circular_iterator operator+(circular_iterator lhs, uint rhs)
                {
                    lhs += rhs;
                    return lhs; 
                }

                /* both iterators need to point within the same buffer and assuming you know which is leading and which lagging,
                this will return an integer which will be the distance between the two iterators */
                friend difference_type distance(const circular_iterator & lagging, const circular_iterator & leading)
                {
                    difference_type diff = leading._current - lagging._current;
                    return diff >= 0 ? diff : diff + (lagging._end - lagging._begin);
                }

                friend bool operator== (const circular_iterator& a, const circular_iterator& b) { return a._current == b._current; };
                friend bool operator!= (const circular_iterator& a, const circular_iterator& b) { return a._current != b._current; };

                //private:
                /* _begin is the first byte of the container, _end is one past the last byte of the container, 
                _current is in the interval [_begin, _end) */
                bytes::iterator _begin, _end, _current;
            };

            protected:

            /* returns an iterator that points to the beginning of the _rx_buffer, store this in
            member variable at init and use it to access the buffer */
            circular_iterator rx_buffer_begin() {return circular_iterator(_rx_buffer, _rx_buffer.begin());}
            /* returns an iterator that points to the last byte written within the _rx_buffer */
            circular_iterator rx_buffer_latest()
            {
            	bytes::iterator it{_write_it};
            	if (_postpone_by_one)
            	{
            		if (--it < _rx_buffer.begin())
            			it = _rx_buffer.end() - 1;
            	}
            	return circular_iterator(_rx_buffer, it);
            }
            /* returns a pointer that points to the byte to be written within the _rx_buffer, use this
            to provide a buffer for single byte interrupt receive */
            inline bytes::pointer rx_buffer_future_write()
            {
            	/* enable postpone, this is important because we are merely providing a buffer
            	to be written to, so the rx_buffer_latest cannot assume that it holds the latest value */
            	_postpone_by_one = true;
            	rx_buffer_advance();
            	return _write_it;
            }
            /* simple assign into the receive buffer */
            inline void put_single_received(byte b)
            {
            	_postpone_by_one = false;
            	rx_buffer_advance();
            	*_write_it = b;
            }

            /* advances the buffer pointer by one, wraps if necessary, call this in receive complete interrupt */
			inline void rx_buffer_advance()
			{
				_write_it = _write_it + 1;
                _byte_count = _byte_count + 1;
				if (_write_it >= _rx_buffer.end())
					_write_it = _rx_buffer.begin();
			}

            bytes::size_type rx_buffer_size() const
            {
                return _rx_buffer.size();
            }

            bytes _rx_buffer;
            volatile bytes::iterator _write_it;
            volatile uint _byte_count;
            volatile bool _postpone_by_one;
        };

        
        
        
        
        template<class Header, class Footer>
        class buffered_parser_interface : public buffered_interface
        {
            using parent = buffered_parser_interface<Header, Footer>;

            public:

            using preamble_type = byte;
            const preamble_type preamble = (preamble_type)0x55;
            const typename Header::size_type preamble_length = 2;

            buffered_parser_interface(interface_identifier iid, address_type address, address_type broadcast_address, 
                uint max_queue_size, uint buffer_size, uint max_fragment_size):
                    buffered_interface(iid, address, broadcast_address, max_queue_size, buffer_size), _max_fragment_size(max_fragment_size)
            {
                _read = rx_buffer_begin();
                _last_byte_count = _byte_count;
            }

            bytes::size_type max_data_size() const noexcept {return _max_fragment_size - (sizeof(Header) + sizeof(Footer) + preamble_length);}
            prealloc_size minimum_prealloc() const noexcept {return prealloc_size(sizeof(Header) + preamble_length, sizeof(Footer));}
            
            protected:

            virtual void do_single_receive() {}

            bytes::size_type do_receive() noexcept
            {
                do_single_receive();
                /* while we are trying to parse the buffer, the ISR is continually filling it
                _write is the position of the last byte written, so we can read up to that point
                since this way we cannot possibly collide with the ISR */
                auto read = _read;
                auto write = rx_buffer_latest();

                /* number of loaded bytes since the last call of this function */
                uint loaded = _last_byte_count <= _byte_count ? (_byte_count - _last_byte_count) : 
                    (std::numeric_limits<uint>::max() - _last_byte_count + _byte_count >= rx_buffer_size());
                _last_byte_count = _byte_count;
                /* log the calculated amount */
                log_received_count(loaded);
                /* rx_buffer may have overflowed since the last call, so our _read does not point
                where the rest of the parser assumes */
                if (loaded >= rx_buffer_size())
                {
                    _read = read = write;
#ifdef SP_BUFFERED_CRITICAL
                    std::cout << "do_receive: buffer overflow" << '\n';
#endif               
                }

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
                                    //bytes b = parsers::byte_copy(fragment_start, fragment_start + fragment_size); //FIXME should be a function
                                    auto fragment_end = fragment_start + fragment_size, it = fragment_start;
                                    bytes b(distance(fragment_start, fragment_end));
                                    for (uint pos = 0; pos < b.size() && it != fragment_end; ++it, ++pos)
                                        b[pos] = *it;

#ifdef SP_BUFFERED_DEBUG
                                    std::cout << "do_receive parse_fragment gets: " << b << std::endl;
#endif
                                    /* attempt the parsing */
                                    if (auto f = parsers::parse_fragment<Header, Footer>(std::move(b), *this))
                                    {
                                        put_received(std::move(*f));
                                        /* parsing succeeded, finally move the read pointer, we do not include the
                                        preamble length here because we don't necessarily know how long it was originally */
                                        _read = read + fragment_size;
#ifdef SP_BUFFERED_DEBUG
                                        std::cout << "do_receive after parse" << std::endl;
#endif
                                    }
                                    else
                                    {
                                        /* parsing failed, move by one because there is no need to try and parse this again */
                                        _read = read + 1;
#ifdef SP_BUFFERED_WARNING
                                        std::cout << "do_receive parse failed" << std::endl;
#endif
                                    }
                                    goto END;
                                }
                                else
                                {
                                    /* once again we just failed the distance check for the whole fragment this time, 
                                    we cannot save the read position because the Header check could be wrong as well */
                                    _read = read;
#ifdef SP_BUFFERED_WARNING
                                    std::cout << "do_receive distance fragment" << std::endl;
#endif
                                    goto END;
                                }
                            }
                            else
                            {
                                /* we failed the size valid check, so this is either a corrupted Header or it's not a Header
                                at all, move the read pointer past this and try again */
                                _read = read = fragment_start;
#ifdef SP_BUFFERED_WARNING
                                std::cout << "do_receive invalid size: " << _read._current - _read._begin << " of " << this->_write_it - _read._begin << std::endl;
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
#endif
                            goto END;
                        }
                    }
                }
                END:
#ifdef SP_BUFFERED_DEBUG
                std::cout << "do_receive returning at: " << _read._current - _read._begin << " of " << this->_write_it - _read._begin << std::endl;
#endif
                return distance(_read, write);
            }

            bytes serialize_fragment(fragment && p) const 
            {
                /* check if the data() has enough capacity, we should never really get here //TODO consider removing */
                if (p.data().capacity_back() < sizeof(Footer) || p.data().capacity_front() < sizeof(Header) + parent::preamble_length)
                {
#ifdef SP_BUFFERED_CRITICAL
                    std::cout << "inadequate fragment.data().capacity() in serialize_fragment: back " << p.data().capacity_back() << ", front " << p.data().capacity_front() << std::endl;
#endif
                    p.data().reserve(sizeof(Header) + parent::preamble_length, sizeof(Footer));
                }
                /* Header */
                p.data().push_front(to_bytes(Header(p)));
                /* preamble */
                auto pr = bytes(parent::preamble_length);
                pr.set(parent::preamble);
                p.data().push_front(pr);
                /* Footer */
                p.data().push_back(to_bytes(Footer(
                    p.data().begin() + parent::preamble_length, p.data().end()
                )));
#ifdef SP_BUFFERED_DEBUG
                std::cout << "serialize_fragment returning: " << p.data() << std::endl;
#endif
                /* move the data out of the packet and return it as an r-value,
                so it is obvious that we want to move it out of the function */
                return bytes(std::move(p.data()));
            }

            buffered_interface::circular_iterator _read;
            uint _max_fragment_size, _last_byte_count;
        };
    }
} // namespace sp


#endif

