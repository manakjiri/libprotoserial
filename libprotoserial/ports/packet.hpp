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

#ifndef _SP_PORTS_PACKET
#define _SP_PORTS_PACKET

#include "libprotoserial/fragmentation/transfer.hpp"
#include "libprotoserial/ports/headers.hpp"

namespace sp
{
    struct packet_metadata : public transfer_metadata
    {
        /* as with interface::address_type this is a type that can hold all used port_type types */
        using port_type = uint;

        static constexpr port_type invalid_port = 0;

        packet_metadata(address_type src, address_type dst, interface_identifier iid, time_point timestamp_creation, 
            id_type id, id_type prev_id, port_type src_port, port_type dst_port) :
                transfer_metadata(src, dst, iid, timestamp_creation, id, prev_id),
                _src_port(src_port), _dst_port(dst_port) {}

        //packet_metadata(address_type src, interface_identifier iid, )

        packet_metadata(transfer_metadata & tm, port_type src_port, port_type dst_port) :
            packet_metadata(tm.source(), tm.destination(), tm.interface_id(), tm.timestamp_creation(), tm.get_id(),
            tm.get_prev_id(), src_port, dst_port) {}

        packet_metadata():
            transfer_metadata(), _src_port(invalid_port), _dst_port(invalid_port) {}

        constexpr port_type source_port() const {return _src_port;}
        constexpr port_type destination_port() const {return _dst_port;}

        constexpr void set_source_port(port_type p) {_src_port = p;}
        constexpr void set_destination_port(port_type p) {_dst_port = p;}

        packet_metadata create_response_packet_metadata() const
        {
            return packet_metadata(destination(), source(), interface_id(), 
                clock::now(), global_id_factory.new_id(interface_id()), get_id(), 
                destination_port(), source_port()
            );
        }

        constexpr bool is_response_packet(const packet_metadata & pm) const 
        {
            return is_response_transfer(pm) && pm.source_port() == destination_port();
        }

        transfer_metadata get_transfer_metadata() const
        {
            return transfer_metadata(*static_cast<const transfer_metadata*>(this));
        }

#ifdef SP_ENABLE_IOSTREAM
        std::ostream& print(std::ostream& os) const
        {
            transfer_metadata::print(os);
            os << ", spt: " << (int)source_port() << ", dpt: " << (int)destination_port();
            return os;
        }
#endif

        protected:
        port_type _src_port, _dst_port;
    };

    struct packet : public packet_metadata
    {
        using data_type = transfer::data_type;
        
        packet() = default;

        packet(packet_metadata pm, data_type d):
            packet_metadata(std::move(pm)), _data(std::move(d)) {}

        packet(transfer && t, port_type src_port = 0, port_type dst_port = 0) :
            _data(std::move(t.data())), packet_metadata(t.source(), t.destination(), t.interface_id(), 
            t.timestamp_creation(), t.get_id(), t.get_prev_id(), src_port, dst_port) {}
        
        template<typename Header>
        packet(transfer && t, const Header & h) :
            packet(std::move(t), h.source, h.destination) {}

        packet(const packet &) = default;
        packet(packet &&) = default;
        packet & operator=(const packet &) = default;
        packet & operator=(packet &&) = default;
        
        constexpr const data_type& data() const noexcept {return _data;}
        constexpr data_type& data() noexcept {return _data;}

        packet create_response_packet() const
        {
            return packet(create_response_packet_metadata(), data_type());
        }

        /* checks that the packet contains all the required address, port and id numbers
        as well as non-empty data field */
        bool is_transmit_ready() const noexcept
        {
            /* source address is filled in by the interface, source port is filled 
            in by the ports_handler */
            return !data().is_empty() && destination() != invalid_address && 
                get_id() != invalid_id && destination_port() != invalid_port;
        }

        private:
        data_type _data;
    };
}

#ifdef SP_ENABLE_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::packet& p) 
{
    p.print(os);
    os << ", " << p.data();
    return os;
}
#endif

#endif