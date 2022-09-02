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
        fragmentation_handler::id_type types
        the fragment id is used to uniquely identify a fragment transfer together with the destination and source
        addresses and the interface name. It is issued by the transmitting side of the fragment */
        using id_type = uint8_t;
        /* index of a fragment within a transfer, starts with 1, index 0 signals invalid index */
        using index_type = uint8_t;

        static constexpr id_type invalid_id = 0; //FIXME should this be here?
        static constexpr index_type invalid_index = 0;

        transfer_metadata(address_type src, address_type dst, interface_identifier iid, 
            time_point timestamp_creation, id_type id, id_type prev_id) :
                fragment_metadata(src, dst, iid, timestamp_creation), _id(id), 
                _prev_id(prev_id) {}

        transfer_metadata(const fragment_metadata & fm, id_type id, id_type prev_id = invalid_id) :
            transfer_metadata(fm.source(), fm.destination(), fm.interface_id(), 
            fm.timestamp_creation(), id, prev_id) {}

        transfer_metadata() :
            fragment_metadata(), _id(invalid_id), _prev_id(invalid_id) {}

        transfer_metadata(const transfer_metadata &) = default;
        transfer_metadata(transfer_metadata &&) = default;
        transfer_metadata & operator=(const transfer_metadata &) = default;
        transfer_metadata & operator=(transfer_metadata &&) = default;

        constexpr id_type get_id() const {return _id;}
        constexpr id_type get_prev_id() const {return _prev_id;}
        
        /* use only once for creating actual response, each transfer only holds one next_id */
        transfer_metadata create_response_transfer_metadata() const
        {
            //TODO should this populate the interface and so forth?
            return transfer_metadata(destination(), source(), interface_id(), 
                clock::now(), global_id_factory.new_id(interface_id()), get_id()
            );
        }

        /* returns the fragment portion of the metadata */
        fragment_metadata get_fragment_metadata() const
        {
            return fragment_metadata(*static_cast<const fragment_metadata*>(this));
        }

        /* check if the given fragment is a response to this fragment */
        constexpr bool is_response_transfer(const transfer_metadata& tm) const
        {
            return is_response_fragment(tm) && get_id() == tm.get_prev_id();
        }

        protected:
        id_type _id, _prev_id;
    };

    struct transfer : public transfer_metadata
    {
        using data_type = fragment::data_type;

        transfer() = default;

        transfer(transfer_metadata && metadata, data_type && data):
            transfer_metadata(std::move(metadata)), _data(std::move(data)) {}

        transfer(interface_identifier iid, address_type dst, id_type prev_id = 0):
            transfer_metadata(0, dst, iid, clock::now(), global_id_factory.new_id(iid), prev_id) {}

        transfer(const transfer &) = default;
        transfer(transfer &&) = default;
        transfer & operator=(const transfer &) = default;
        transfer & operator=(transfer &&) = default;

        constexpr const data_type& data() const noexcept {return _data;}
        constexpr data_type& data() noexcept {return _data;}

        /* use only once for creating actual response, each transfer only holds one next_id */
        transfer create_response_transfer() const
        {
            return transfer(create_response_transfer_metadata(), data_type());
        }

#ifdef SP_ENABLE_IOSTREAM
        friend std::ostream& operator<<(std::ostream& os, const transfer & t) 
        {
            os << "dst: " << (int)t.destination() << ", src: " << (int)t.source();
            os << ", int: " << t.interface_id();
            os << ", id: " << (int)t.get_id() << ", prev_id: " << (int)t.get_prev_id();
            os << ", " << t.data();
            return os;
        }

        friend std::ostream& operator<<(std::ostream& os, const transfer * t) 
        {
            if (t) os << *t; else os << "null transfer";
            return os;
        }
#endif

        protected:
        data_type _data;
    };
}

#endif
