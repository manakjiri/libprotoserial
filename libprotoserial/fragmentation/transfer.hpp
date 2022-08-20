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
        using id_type = uint8_t;
        /* index of a fragment within a transfer, starts with 1, index 0 signals invalid index */
        using index_type = uint8_t;

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

        fragment_metadata get_fragment_metadata() const
        {
            return fragment_metadata(*reinterpret_cast<const fragment_metadata*>(this));
        }

        protected:
        id_type _id, _prev_id;
    };

    struct transfer : public transfer_metadata, public sp_object
    {
        using data_type = fragment::data_type;

        /* constructor used when the fragmentation_handler receives the first piece of the transfer */
        template<class Header>
        transfer(interface_identifier iid, const Header & h) :
            transfer_metadata(0, 0, iid, clock::now(), h.get_id(), h.get_prev_id()) {}

        transfer(interface_identifier iid, id_type prev_id = 0):
            transfer_metadata(0, 0, iid, clock::now(), global_id_factory.new_id(iid), prev_id) {}
        transfer(const interface & i, id_type prev_id = 0):
            transfer_metadata(0, 0, i.interface_id(), clock::now(), global_id_factory.new_id(i.interface_id()), prev_id) {}

        transfer(transfer_metadata && metadata, data_type && data):
            transfer_metadata(std::move(metadata)), _data(std::move(data)) {}

        transfer(const transfer &) = default;
        transfer(transfer &&) = default;
        transfer & operator=(const transfer &) = default;
        transfer & operator=(transfer &&) = default;

        const data_type& data() const noexcept {return _data;}
        data_type& data() noexcept {return _data;}

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
