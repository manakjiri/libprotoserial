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

#include <libprotoserial/services/common/service_base.hpp>
#include <libprotoserial/services/common/command_io.hpp>
#include <libprotoserial/services/common/command_status.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>

#include <list>
#include <functional>

namespace sp
{
    class command_client : public service_base
    {
        public:
        using command_args = command_io_encoder;
        using command_output = command_io_parser;
        using callback_type = std::function<void(command_status, std::optional<command_output>)>;

        private:
        class command_connection : public service_base
        {
            callback_type _callback;
            
            public:
            command_connection(ports_handler & l, callback_type callback) :
                service_base(l, l.get_free_port()), _callback(std::move(callback)) {}

            /* implements service_base::receive */
            /* this will receive the output of the executed command on the other end
            at the time of reception we may not know yet from which port this data
            will originate since the packet from the command_server may get here later
            //FIXME for now we assume that any data we receive here is meant for us */
            void receive(packet p)
            {
                //std::cout << "command_connection: " << p << std::endl; 
                //TODO
                /* _packet = std::move(p); */
            }

            /* void set_origin_port(packet::port_type p)
            {
                _origin_port = p;
            }

            void handle()
            {
                if (_origin_port != packet::invalid_port && _packet)
                {
                    fire the callback
                }
            } */
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
            /* create_packet() creates a packet with data where data.size() == 128 in this case
            encode_cbor calls push_back, so we need to shrink the apparent size to 0 */
            req.data().shrink(0, 128);
            jsoncons::cbor::encode_cbor(query, req.data());

            transmit(std::move(req));
        }

        void send_command(packet::address_type addr, interface_identifier iid, packet::port_type port, 
            std::string cmd)
        {
            send_command(addr, iid, port, cmd, command_args(), 
                [](command_status s, std::optional<command_output> o){});
        }

        /* implements service_base::receive */
        /* this function should only ever handle data coming from a command_server instance
        on the other end, to which a request was sent using the send_command function */
        void receive(packet p)
        {
            //std::cout << "command_client: " << p << std::endl;

            //FIXME this will call terminate if the data is corrupted or incorrect
            const jsoncons::json j = jsoncons::cbor::decode_cbor<jsoncons::json>(p.data());
            std::cout << jsoncons::pretty_print(j) << std::endl;

            if (!j.is_object())
                return;

            if (j.contains("err"))
            {
                /* something gone wrong on the other end */
                
                //TODO
            }
            //TODO use the id for this, makes much more sense
            else if (j.contains("cmd") && j.contains("port"))
            {


            }
        }
    };
}


#endif



