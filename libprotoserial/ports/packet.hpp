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

namespace sp
{
    struct packet_metadata : public transfer_metadata
    {
        /* as with interface::address_type this is a type that can hold all used port_type types */
        using port_type = uint;

        packet_metadata(address_type src, address_type dst, interface * i, time_point timestamp_creation, 
            id_type id, id_type prev_id, port_type src_port, port_type dst_port) :
                transfer_metadata(src, dst, i, timestamp_creation, id, prev_id),
                _src_port(src_port), _dst_port(dst_port) {}

        port_type source_port() const {return _src_port;}
        port_type destination_port() const {return _dst_port;}

        protected:
        port_type _src_port, _dst_port;
    };

    struct packet : public packet_metadata, virtual public transfer
    {
        

    };

}

#endif