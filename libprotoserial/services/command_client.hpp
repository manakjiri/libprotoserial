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


#ifndef _SP_SERVICES_COMMANDCLIENT
#define _SP_SERVICES_COMMANDCLIENT

#include <libprotoserial/services/command_server.hpp>

#include <list>
#include <functional>

namespace sp
{
    class command_client : public service_base
    {
        public:
        using command_args = detail::command_io_encoder;
        using command_output = detail::command_io_parser;
        using request_status = command_server::request_status;
        using callback_type = std::function<void(request_status, std::optional<command_output>)>;

        private:
        class command_connection : public service_base
        {
            callback_type _callback;
            
            public:
            command_connection(ports_handler & l, callback_type callback) :
                service_base(l, l.get_free_port()), _callback(std::move(callback)) {}

            void receive(packet p)
            {
                //TODO
            }
        };
        
        std::list<command_connection> _connections;
        ports_handler & _ports;

        public:

        command_client(ports_handler & l, port_type port):
            service_base(l, port), _ports(l) {}

        void send_command(packet::address_type addr, interface_identifier iid, packet::port_type port, 
            std::string cmd, command_args args, callback_type callback)
        {
            const auto & conn = _connections.emplace_back(_ports, std::move(callback));
            
            jsoncons::json query;
            query["cmd"] = cmd;
            query["args"] = args.to_json();
            query["port"] = conn.get_port();

            packet req = create_packet(128, addr, iid, port); //FIXME better size estimate
            jsoncons::cbor::encode_cbor(query, req.data());

            transmit(std::move(req));
        }
    };
}


#endif



