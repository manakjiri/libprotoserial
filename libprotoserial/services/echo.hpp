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


#ifndef _SP_SERVICES_ECHO
#define _SP_SERVICES_ECHO

#include <libprotoserial/services/service_base.hpp>

namespace sp
{
    /* test service */
    class echo_service : public service_base
    {
        public:
        echo_service(ports_handler & l, port_type port) :
            service_base(l, port) {}

        /* implements service_base::receive */
        void receive(packet p)
        {
            auto resp = p.create_response_packet();
            resp.data() = std::move(p.data());
            transmit(std::move(resp));
        }
    };
}


#endif



