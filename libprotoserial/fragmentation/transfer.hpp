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
#include "libprotoserial/fragmentation/id_factory.hpp"

namespace sp
{
    class fragmentation_handler;

    struct transfer_metadata : public fragment_metadata
    {
        /* as with interface::address_type this is a type that can hold all used 
        fragmentation_handler::id_type types */
        using id_type = uint;
        using index_type = uint;

        transfer_metadata(address_type src, address_type dst, interface_identifier iid, 
            time_point timestamp_creation, id_type id, id_type prev_id) :
                fragment_metadata(src, dst, iid, timestamp_creation), _id(id), 
                _prev_id(prev_id) {}

        transfer_metadata(const transfer_metadata &) = default;
        transfer_metadata(transfer_metadata &&) = default;
        transfer_metadata & operator=(const transfer_metadata &) = default;
        transfer_metadata & operator=(transfer_metadata &&) = default;

        /* the fragment id is used to uniquely identify a fragment transfer together with the destination and source
        addresses and the interface name. It is issued by the transmittee of the fragment */
        id_type get_id() const {return _id;}
        id_type get_prev_id() const {return _prev_id;}

        /* checks if p's addresses and interface match the transfer's, this along with id match means that p 
        should be part of this transfer */
        bool match(const fragment & p) const 
            {return p.destination() == destination() && p.source() == source();}

        /* checks p's addresses as a response to this transfer and interface match the transfer's */
        bool match_as_response(const fragment & p) const 
            {return p.source() == destination();}

        /* use only once for creating actual response, each transfer only holds one next_id */
        transfer_metadata create_response() 
        {
            return transfer_metadata(destination(), source(), interface_id(), 
                clock::now(), global_id_factory.new_id(interface_id()), get_id()
            );
        }

        protected:
        id_type _id, _prev_id;
    };

    struct transfer_data
    {
        using data_type = fragment::data_type;

        transfer_data():
            _timestamp_modified(clock::now()) {}

        transfer_data(std::vector<bytes> && data) :
            _data(std::move(data)), _timestamp_modified(clock::now()) {}

        transfer_data(std::size_t reserve):
            _data(reserve), _timestamp_modified(clock::now()) {}

        transfer_data(const transfer_data &) = default;
        transfer_data(transfer_data &&) = default;

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

            data_iterator(const transfer_data * p, bool is_begin) : 
                _fragment(p) 
            {
                if (is_begin)
                {
                    /* initialize the iterators to the first byte of the first fragment */
                    _ifragment = _fragment->_begin();
                    if (_ifragment != _fragment->_end())
                    {
                        _ifragment_data = _ifragment->begin();
                        while(_ifragment_data == _ifragment->end() && _ifragment != _fragment->_last())
                        {
                            ++_ifragment;
                            _ifragment_data = _ifragment->begin();
                        }
                    }
                    else
                        _ifragment_data = nullptr;
                }
                else
                {
                    /* initialize the iterators to the end of data of the last fragment */
                    _ifragment = _fragment->_last();
                    if (_ifragment != _fragment->_end())
                        _ifragment_data = _ifragment->end();
                    else
                        _ifragment_data = nullptr;
                }
            }

            data_iterator(const data_iterator &) = default;
            data_iterator(data_iterator &&) = default;
            data_iterator & operator=(const data_iterator &) = default;
            data_iterator & operator=(data_iterator &&) = default;

            reference operator*() const { return *_ifragment_data; }
            pointer operator->() const { return _ifragment_data; }

            // Prefix increment
            data_iterator& operator++() 
            {
                /* we want to increment by one, try to increment the data iterator
                within the current fragment */
                ++_ifragment_data;
                /* when we are at the end of data of current fragment, we need
                to advance to the next fragment and start from the beginning of
                its data */
                if (_ifragment_data == _ifragment->end() && _ifragment != _fragment->_last())
                {
                    do {
                        ++_ifragment;
                        _ifragment_data = _ifragment->begin();
                    } while (_ifragment_data == _ifragment->end() && _ifragment != _fragment->_last());
                }
                return *this;
            }

            data_iterator& operator+=(uint shift)
            {
                //HACK make more efficient?
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
                { return a._ifragment_data == b._ifragment_data && a._ifragment == b._ifragment; };
            friend bool operator!= (const data_iterator& a, const data_iterator& b) 
                { return a._ifragment_data != b._ifragment_data || a._ifragment != b._ifragment; };

            private:
            const transfer_data * _fragment;
            std::vector<data_type>::const_iterator _ifragment;
            data_type::const_iterator _ifragment_data;
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

        transfer_metadata::time_point timestamp_modified() const {return _timestamp_modified;}

        protected:

        void refresh() {_timestamp_modified = clock::now();}

        std::vector<data_type> _data;
        transfer_metadata::time_point _timestamp_modified;
    };

    /* there should never be a need for the user to construct this, it is provided by the
        * fragmentation_handler as fully initialized object ready to be used, user should never
        * need to use function that start with underscore 
        * 
        * there are two modes of usage from within the fragmentation_handler:
        * 1)  transfer is constructed using the transfer(const Header & h, fragmentation_handler * handler)
        *     constructor when fragmentation_handler encounters a new ID and such transfer is put into the
        *     _incoming_transfers list, where it is gradually filled with incoming fragments from the interface.
        *     This is a mode where transfer's internal vector gets preallocated to a known number, so functions
        *     _is_complete(), _missing_fragment() and _missing_fragments_total() make sense to call.
        * 
        * 2)  transfer is created using the new_transfer() function, where it is constructed by the
        *     transfer(fragmentation_handler * handler, id_type id, id_type prev_id = 0) constructor on user
        *     demand. This transfer will, presumably, be transmitted some time in the future. The internal
        *     vector is empty and gets dynamically larger as user uses the push_front(bytes) and push_back(bytes)
        *     functions. 
        *     In contrast to the first scenario data will no longer satisfy the data_size() <= max_fragment_size()
        *     property, which is required for transmit, so function the _get_fragment() is used by the 
        *     fragmentation_handler to create fragments that do.
        */
    struct transfer : public transfer_metadata, public transfer_data
    {
        /* constructor used when the fragmentation_handler receives the first piece of the transfer */
        template<class Header>
        transfer(interface_identifier iid, const Header & h) :
            transfer_metadata(0, 0, iid, clock::now(), h.get_id(), h.get_prev_id()),
            transfer_data(h.fragments_total()) {}

        /* constructor used by the fragmentation_handler in new_transfer */
        transfer(interface_identifier iid, id_type prev_id = 0):
            transfer_metadata(0, 0, iid, clock::now(), global_id_factory.new_id(iid), prev_id) {}

        transfer(transfer_metadata && metadata, transfer_data && data):
            transfer_metadata(std::move(metadata)), transfer_data(std::move(data)) {}

        transfer(const transfer &) = default;
        transfer(transfer &&) = default;
        transfer & operator=(const transfer &) = default;
        transfer & operator=(transfer &&) = default;

        void _push_back(const fragment & p) {refresh(p); _data.push_back(p.data());}
        void _push_back(fragment && p) {refresh(p); _data.push_back(std::move(p.data()));}
        void _assign(index_type fragment_pos, const fragment & p) {refresh(p); _data.at(fragment_pos - 1) = p.data();}
        void _assign(index_type fragment_pos, fragment && p) {refresh(p); _data.at(fragment_pos - 1) = std::move(p.data());}

        transfer_metadata get_metadata() const 
        {
            return transfer_metadata(*reinterpret_cast<const transfer_metadata*>(this));
        }

#ifndef SP_NO_IOSTREAM
        friend std::ostream& operator<<(std::ostream& os, const transfer & t) 
        {
            os << "dst: " << t.destination() << ", src: " << t.source();
            os << ", int: " << t.interface_id();
            os << ", id: " << t.get_id() << ", prev_id: " << t.get_prev_id();
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

        void refresh(const fragment & p)
        {
            transfer_data::refresh();
            _source = p.source();
            _destination = p.destination();
            _interface_id = p.interface_id();
        }
    };
}

#endif
