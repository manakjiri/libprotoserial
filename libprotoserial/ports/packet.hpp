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

        packet_metadata(address_type src, address_type dst, interface_identifier iid, time_point timestamp_creation, 
            id_type id, id_type prev_id, port_type src_port, port_type dst_port) :
                transfer_metadata(src, dst, iid, timestamp_creation, id, prev_id),
                _src_port(src_port), _dst_port(dst_port) {}

        //packet_metadata(address_type src, interface_identifier iid, )

        port_type source_port() const {return _src_port;}
        port_type destination_port() const {return _dst_port;}

        void set_destination_port(port_type p) {_dst_port = p;}

        packet_metadata create_response()
        {
            return packet_metadata(destination(), source(), interface_id(), 
                clock::now(), global_id_factory.new_id(interface_id()), get_id(), 
                destination_port(), source_port()
            );
        }

        bool match_as_response(const packet_metadata & p) const 
        {
            return p.source() == destination() && p.interface_id() == interface_id() && 
                p.get_prev_id() == get_id() && p.source_port() == destination_port();
        }

        protected:
        port_type _src_port, _dst_port;
    };

    struct packet : public transfer, public packet_metadata
    {
        packet(transfer && t, port_type src_port = 0, port_type dst_port = 0) :
            transfer(std::move(t)), packet_metadata(t.source(), t.destination(), t.interface_id(), 
            t.timestamp_creation(), t.get_id(), t.get_prev_id(), src_port, dst_port) {}
        
        packet(transfer && t, const headers::ports_8b & h) :
            packet(std::move(t), h.source, h.destination) {}

        transfer to_transfer() {return transfer(std::move(*this));}
    };

}

#endif