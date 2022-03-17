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


#ifndef _SP_FRAGMENTATION_TRANSFER
#define _SP_FRAGMENTATION_TRANSFER

#include "libprotoserial/interface.hpp"
#include "libprotoserial/clock.hpp"

namespace sp
{
    class fragmentation_handler;

    class transfer_metadata : public interface::packet_metadata
    {
        public:

        using data_type = interface::packet::data_type;

        /* as with interface::address_type this is a type that can hold all used 
        fragmentation_handler::id_type types */
        using id_type = uint;
        using index_type = uint;

        transfer_metadata(address_type src, address_type dst, const interface * i, clock::time_point timestamp_creation, 
            clock::time_point timestamp_modified, fragmentation_handler * handler, id_type id, id_type prev_id) :
                interface::packet_metadata(src, dst, i, timestamp_creation), _timestamp_modified(timestamp_modified),
                _handler(handler), _id(id), _prev_id(prev_id) {}

        transfer_metadata(const transfer_metadata &) = default;
        transfer_metadata(transfer_metadata &&) = default;
        transfer_metadata & operator=(const transfer_metadata &) = default;
        transfer_metadata & operator=(transfer_metadata &&) = default;

        /* the packet id is used to uniquely identify a packet transfer together with the destination and source
        addresses and the interface name. It is issued by the transmittee of the packet */
        id_type get_id() const {return _id;}
        id_type get_prev_id() const {return _prev_id;}
        const fragmentation_handler * get_handler() const {return _handler;}
        clock::time_point timestamp_modified() const {return _timestamp_modified;}

        /* checks if p's addresses and interface match the transfer's, this along with id match means that p 
        should be part of this transfer */
        bool match(const interface::packet & p) const 
            {return p.destination() == destination() && p.source() == source();}

        /* checks p's addresses as a response to this transfer and interface match the transfer's */
        bool match_as_response(const interface::packet & p) const 
            {return p.source() == destination();}

        protected:
        clock::time_point _timestamp_modified;
        fragmentation_handler * _handler;
        id_type _id, _prev_id;
    };

    /* there should never be a need for the user to construct this, it is provided by the
        * fragmentation_handler as fully initialized object ready to be used, user should never
        * need to use function that start with underscore 
        * 
        * there are two modes of usage from within the fragmentation_handler:
        * 1)  transfer is constructed using the transfer(const Header & h, fragmentation_handler * handler)
        *     constructor when fragmentation_handler encounters a new ID and such transfer is put into the
        *     _incoming_transfers list, where it is gradually filled with incoming packets from the interface.
        *     This is a mode where transfer's internal vector gets preallocated to a known number, so functions
        *     _is_complete(), _missing_fragment() and _missing_fragments_total() make sense to call.
        * 
        * 2)  transfer is created using the new_transfer() function, where it is constructed by the
        *     transfer(fragmentation_handler * handler, id_type id, id_type prev_id = 0) constructor on user
        *     demand. This transfer will, presumably, be transmitted some time in the future. The internal
        *     vector is empty and gets dynamically larger as user uses the push_front(bytes) and push_back(bytes)
        *     functions. 
        *     In contrast to the first scenario data will no longer satisfy the data_size() <= max_packet_size()
        *     property, which is required for transmit, so function the _get_fragment() is used by the 
        *     fragmentation_handler to create packets that do.
        */
    class transfer : public transfer_metadata
    {
        public:

        /* constructor used when the fragmentation_handler receives the first piece of the 
        packet - when new a packet transfer is initiated by other peer. This initial packet
        does not need to be the first fragment, fragmentation_handler is responsible for 
        the correct order of interface::packets within this object */
        template<class Header>
        transfer(const Header & h, fragmentation_handler * handler) :
            transfer_metadata(0, 0, nullptr, clock::now(), clock::now(), 
            handler, h.get_id(), h.get_prev_id()), _hidden_front(0), _data(h.fragments_total()) {}

        /* constructor used by the fragmentation_handler in new_transfer */
        transfer(fragmentation_handler * handler, id_type id, id_type prev_id = 0):
            transfer_metadata(0, 0, nullptr, clock::now(), clock::now(), handler, id, prev_id),
            _hidden_front(0) {}

        transfer(const transfer &) = default;
        transfer(transfer &&) = default;
        transfer & operator=(const transfer &) = default;
        transfer & operator=(transfer &&) = default;
        
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        auto _begin() {return _data.begin();}
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        auto _begin() const {return _data.begin();}
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        auto _end() {return _data.end();}
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        auto _end() const {return _data.end();}
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        auto _last() {return _data.empty() ? _data.begin() : std::prev(_data.end());}
        auto _last() const {return _data.empty() ? _data.begin() : std::prev(_data.end());}

        void _push_back(const interface::packet & p) {refresh(p); _data.push_back(p.data());}
        void _push_back(interface::packet && p) {refresh(p); _data.push_back(std::move(p.data()));}
        void _assign(index_type fragment, const interface::packet & p) {refresh(p); _data.at(fragment - 1) = p.data();}
        void _assign(index_type fragment, interface::packet && p) {refresh(p); _data.at(fragment - 1) = std::move(p.data());}

        /* expose the internal data, used in fragmentation_handler and data_iterator */
        //auto _at(size_t pos) {return _data.at(pos);}
        /* expose the internal data, used in fragmentation_handler and data_iterator */
        //auto _at(size_t pos) const {return _data.at(pos);}
        //auto _size() const {return _data.size();}

        void push_back(const bytes & b) {refresh(); _data.push_back(b);}
        void push_back(bytes && b) {refresh(); _data.push_back(std::move(b));}
        void push_front(const bytes & b) {refresh(); _data.insert(_data.begin(), b);}
        void push_front(bytes && b) {refresh(); _data.insert(_data.begin(), std::move(b));}
        /* this removes the first byte from the internally stored fragments */
        void remove_first()
        {
            for (auto & c : _data)
            {
                if (c)
                {
                    c.shrink(1, 0);
                    break;
                }
            }
        }
        /* this removes the first n bytes from the internally stored fragments, 
        useful for hiding headers */
        void remove_first_n(data_type::size_type n) 
        {
            while (n--) remove_first();
        }

        struct data_iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = data_type::difference_type;
            using value_type        = data_type::value_type;
            using pointer           = data_type::const_pointer; 
            using reference         = data_type::const_reference;

            data_iterator(const transfer * p, bool is_begin) : 
                _packet(p) 
            {
                if (is_begin)
                {
                    /* initialize the iterators to the first byte of the first interface::packet */
                    _ipacket = _packet->_begin();
                    if (_ipacket != _packet->_end())
                    {
                        _ipacket_data = _ipacket->begin();
                        while(_ipacket_data == _ipacket->end() && _ipacket != _packet->_last())
                        {
                            ++_ipacket;
                            _ipacket_data = _ipacket->begin();
                        }
                    }
                    else
                        _ipacket_data = nullptr;
                }
                else
                {
                    /* initialize the iterators to the end of data of the last interface::packet */
                    _ipacket = _packet->_last();
                    if (_ipacket != _packet->_end())
                        _ipacket_data = _ipacket->end();
                    else
                        _ipacket_data = nullptr;
                }
            }

            data_iterator(const data_iterator &) = default;
            data_iterator(data_iterator &&) = default;
            data_iterator & operator=(const data_iterator &) = default;
            data_iterator & operator=(data_iterator &&) = default;

            reference operator*() const { return *_ipacket_data; }
            pointer operator->() const { return _ipacket_data; }

            // Prefix increment
            data_iterator& operator++() 
            {
                /* we want to increment by one, try to increment the data iterator
                within the current interface::packet */
                ++_ipacket_data;
                /* when we are at the end of data of current interface::packet, we need
                to advance to the next interface::packet and start from the beginning of
                its data */
                if (_ipacket_data == _ipacket->end() && _ipacket != _packet->_last())
                {
                    do {
                        ++_ipacket;
                        _ipacket_data = _ipacket->begin();
                    } while (_ipacket_data == _ipacket->end() && _ipacket != _packet->_last());
                }
                return *this;
            }

            data_iterator& operator+=(uint shift)
            {
                //FIXME
                while (shift > 0)
                {
                    this->operator++();
                    --shift;
                }
                return *this; 
            }

            friend data_iterator operator+(data_iterator lhs, uint rhs)
            {
                lhs += rhs;
                return lhs; 
            }

            friend bool operator== (const data_iterator& a, const data_iterator& b) 
                { return a._ipacket_data == b._ipacket_data && a._ipacket == b._ipacket; };
            friend bool operator!= (const data_iterator& a, const data_iterator& b) 
                { return a._ipacket_data != b._ipacket_data || a._ipacket != b._ipacket; };

            private:
            //TODO rename
            const transfer * _packet;
            std::vector<data_type>::const_iterator _ipacket;
            data_type::const_iterator _ipacket_data;
        };

        /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
        auto data_begin() const {return data_iterator(this, true);}
        /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
        auto data_end() const {return data_iterator(this, false);}

        bytes::size_type data_size() const 
        {
            bytes::size_type ret = 0;
            for (auto it = _begin(); it != _end(); ++it) ret += it->size();
            return ret;
        }

        /* copies all the internally stored data into one bytes container and returns it */
        bytes data_contiguous() const
        {
            bytes b(0, 0, data_size());
            for (auto it = _begin(); it != _end(); ++it) b.push_back(*it);
            return b;
        }

        transfer_metadata get_metadata() const 
        {
            return transfer_metadata(*reinterpret_cast<const transfer_metadata*>(this));
        }

#ifndef SP_NO_IOSTREAM
        friend std::ostream& operator<<(std::ostream& os, const transfer & t) 
        {
            os << "dst: " << t.destination() << ", src: " << t.source();
            os << ", int: " << (t.get_interface() ? t.get_interface()->name() : "null");
            os << ", id: " << t.get_id() << ", prev_id: " << t.get_prev_id();
            //auto f = t._fragments_count();
            //os << ", (" << (f - t._missing_fragments_total()) << '/' << f << ")";
            os << ", " << t.data_contiguous();
            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const transfer * t) 
        {
            if (t) os << *t; else os << "null transfer";
            return os;
        }
#endif

        protected:

        //TODO
        void refresh() {_timestamp_modified = clock::now();}
        void refresh(const interface::packet & p)
        {
            refresh();
            _source = p.source();
            _destination = p.destination();
            _interface = p.get_interface();
        }
        
        data_type::size_type _hidden_front;
        std::vector<data_type> _data;
    };
}

#endif
