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


#ifndef _SP_INTERFACE_FRAGMENT
#define _SP_INTERFACE_FRAGMENT

#include "libprotoserial/container.hpp"
#include "libprotoserial/clock.hpp"

namespace sp
{
    class interface;

    class fragment_metadata
    {
        public:
        /* an integer that can hold any used device address, the actual address format 
        is interface specific, address 0 is reserved internally and should never appear 
        in a fragment */
        using address_type = uint;
        using time_point = clock::time_point;

        fragment_metadata(address_type src, address_type dst, interface *i, time_point timestamp_creation):
            _timestamp_creation(timestamp_creation), _interface(i), _source(src), _destination(dst) {}

        fragment_metadata(const fragment_metadata &) = default;
        fragment_metadata(fragment_metadata &&) = default;
        fragment_metadata & operator=(const fragment_metadata &) = default;
        fragment_metadata & operator=(fragment_metadata &&) = default;

        constexpr time_point timestamp_creation() const {return _timestamp_creation;}
        constexpr interface* get_interface() const noexcept {return _interface;}
        constexpr address_type source() const noexcept {return _source;}
        constexpr address_type destination() const noexcept {return _destination;}

        void set_destination(address_type dst) {_destination = dst;}

        protected:
        time_point _timestamp_creation;
        mutable interface *_interface;
        address_type _source, _destination;
    };

    /* interface fragment representation */
    class fragment : public fragment_metadata
    {
        public:

        typedef bytes   data_type;

        fragment(address_type src, address_type dst, data_type && d, interface *i) :
            fragment_metadata(src, dst, i, clock::now()), _data(std::move(d)) {}

        /* this object can be passed to the interface::write() function */
        fragment(address_type dst, data_type && d) :
            fragment((address_type)0, dst, std::move(d), nullptr) {}

        fragment():
            fragment(0, data_type()) {}

        fragment(const fragment &) = default;
        fragment(fragment &&) = default;
        fragment & operator=(const fragment &) = default;
        fragment & operator=(fragment &&) = default;
        
        constexpr const data_type& data() const noexcept {return _data;}
        constexpr data_type& data() noexcept {return _data;}
        constexpr void _complete(address_type src, interface *i) {_source = src; _interface = i;}
        
        bool carries_information() const {return _data && _destination;}
        explicit operator bool() const {return carries_information();}

        protected:
        data_type _data;
    };
}


#endif

