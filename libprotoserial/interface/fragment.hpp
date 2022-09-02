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

#include "libprotoserial/data/container.hpp"
#include "libprotoserial/clock.hpp"
#include "libprotoserial/interface/interface_id.hpp"
#include "libprotoserial/utils/object_id.hpp"

namespace sp
{
    class interface;

    class fragment_metadata : public sp_object
    {
        public:
        /* an integer that can hold any used device address, the actual address format 
        is interface specific, address 0 is reserved internally and should never appear 
        in a fragment */
        using address_type = uint;
        using time_point = clock::time_point;

        static constexpr address_type invalid_address = 0;

        fragment_metadata(address_type src, address_type dst, interface_identifier iid, time_point timestamp_creation):
            _timestamp_creation(timestamp_creation), _interface_id(iid), _source(src), _destination(dst) {}

        fragment_metadata():
            fragment_metadata(invalid_address, invalid_address, interface_identifier(), never()) {}

        fragment_metadata(const fragment_metadata &) = default;
        fragment_metadata(fragment_metadata &&) = default;
        fragment_metadata & operator=(const fragment_metadata &) = default;
        fragment_metadata & operator=(fragment_metadata &&) = default;

        constexpr time_point timestamp_creation() const noexcept {return _timestamp_creation;}
        constexpr interface_identifier interface_id() const noexcept {return _interface_id;}
        constexpr address_type source() const noexcept {return _source;}
        constexpr address_type destination() const noexcept {return _destination;}

        constexpr void set_destination(address_type dst) {_destination = dst;}
        constexpr void set_interface_id(interface_identifier iid) {_interface_id = iid;}

        fragment_metadata create_response_fragment_metadata() const
        {
            return fragment_metadata(destination(), source(), interface_id(), time_point::clock::now());
        }

        //TODO match_as_response fragment formalize
        /* check if the given fragment is a response to this fragment */
        constexpr bool is_response_fragment(const fragment_metadata & fm) const
        {
            return fm.source() == destination() && interface_id() == fm.interface_id();
        }

        protected:
        time_point _timestamp_creation;
        interface_identifier _interface_id;
        address_type _source, _destination;
    };

    /* interface fragment representation */
    class fragment : public fragment_metadata
    {
        public:
        using data_type = bytes;

        fragment() = default;

        fragment(address_type src, address_type dst, data_type && d, interface_identifier iid, 
            time_point t = clock::now()) :
                fragment_metadata(src, dst, iid, t), _data(std::move(d)) {}

        fragment(fragment_metadata && metadata, data_type && d):
            fragment_metadata(std::move(metadata)), _data(std::move(d)) {}

        /* this object can be passed to the interface::transmit() function */
        fragment(address_type dst, data_type d) :
            fragment(invalid_address, dst, std::move(d), interface_identifier()) {}

        fragment(const fragment &) = default;
        fragment(fragment &&) = default;
        fragment & operator=(const fragment &) = default;
        fragment & operator=(fragment &&) = default;
        
        constexpr const data_type& data() const noexcept {return _data;}
        constexpr data_type& data() noexcept {return _data;}
        
        //TODO remove
        constexpr void complete(address_type src, interface_identifier iid) {_source = src; _interface_id = iid;}
        
        constexpr bool carries_information() const {return _data && _destination != invalid_address;}
        explicit constexpr operator bool() const {return carries_information();}

        fragment create_response_fragment() const
        {
            return fragment(create_response_fragment_metadata(), data_type());
        }

        protected:
        data_type _data;
    };
}

bool operator==(const sp::fragment & lhs, const sp::fragment & rhs)
{
    return lhs.interface_id() == rhs.interface_id() && lhs.source() == rhs.source() && 
        lhs.destination() == rhs.destination() && lhs.data() == rhs.data();
}

bool operator!=(const sp::fragment & lhs, const sp::fragment & rhs)
{
    return !(lhs == rhs);
}

#ifdef SP_ENABLE_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::fragment& p) 
{
    os << "dst: " << p.destination() << ", src: " << p.source();
    os << ", int: " << p.interface_id();
    os << ", " << p.data();
    return os;
}
#endif


#endif

