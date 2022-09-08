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


#ifndef _SP_SERVICES_COMMANDSERVER
#define _SP_SERVICES_COMMANDSERVER

#include <libprotoserial/services/service_base.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>

#include <string>
#include <unordered_map>


namespace sp
{
namespace detail
{
    class command_io_parser
    {
        jsoncons::json _query;

        public:
        command_io_parser(jsoncons::json query) :
            _query(std::move(query)) {}

        /* get the n'th argument of specified type, starting from index 0, 
        allowed types are int, double, bool, std::string, sp::bytes
        you will get a build error if these types are not used */
        template<typename ArgType> 
        std::optional<ArgType> at(const std::size_t n) const
        {
            /* "args" should be an array and it should contain the index the user is asking for */
            if (!_query.is_array() || n >= _query.size())
                return std::nullopt;
            
            const auto & arg = _query[n];
            /* do a runtime type check before we try to interpret the value 
            using the as<> function, which throws when it fails, which we do not want */
            if constexpr(std::is_same<ArgType, bytes>::value)
            {
                if (arg.is_byte_string())
                    return arg.as<ArgType>();
            }
            else
            {
                if (arg.is<ArgType>())
                    return arg.as<ArgType>();
            }

            return std::nullopt;
        }
    };

    class command_io_encoder
    {
        jsoncons::json _query;
        
        public:
        command_io_encoder() :
            _query(jsoncons::json_array_arg) {}

        template<typename T>
        void push_back(T val)
        {
            _query.push_back(std::move(val));
        }

        jsoncons::json to_json() const
        {
            return _query;
        }
    };
}


    class command_server : public service_base
    {
        public:

        /* TODO add err_arg0, err_arg1, ...? the service could return this in a code 
        with offset, ie. 100 + static_cast<int>(exit_code) */
        enum class exit_status { DONE, CONTINUE, ERR_ARGS, ERR_RUNTIME };

        using command_args = detail::command_io_parser;
        using command_output = detail::command_io_encoder;

        class command_base : public service_base
        {
            command_server * _service = nullptr;
            bool _is_running = false;

            public:
            using enum exit_status;

            /* the base class needs to be default constructible for ease of use
            we do not want to pass the args to the factory function which then
            would need to deal with them */
            command_base() {}

            /* TODO we will need to turn this into a full blown service_base
            specialization, so we need to pass additional things anyway.
            use the start function as a "secondary constructor" and pass the args,
            the port and the command_server pointer */
            exit_status start(command_server * service, jsoncons::json query)
            {
                _service = service;

                exit_status ret = setup(command_args(std::move(query["args"])));

                if (ret == CONTINUE)
                    _is_running = true;

                return ret;
            }

            bool is_running() const
            {
                return _is_running;
            }

            protected:

            void output(command_output out) {}
            void print(std::string str) {}

            /* do not store the reference to args, they get destroyed after
            the setup function returns */
            virtual exit_status setup(const command_args & args) = 0;
            /* optional, if setup() returns DONE, this never gets called
            any data received is forwarded to the command in raw form */
            virtual exit_status run(bytes data) {return DONE;}
            
            public:
            /* destructor serves as the cleanup function, naturally */
            virtual ~command_base() {}

            private:
            void receive(packet p)
            {
                //TODO
            }
        };

        using factory_type = std::function<std::unique_ptr<command_base>(void)>;

        private:
        
        std::unordered_map<std::string, factory_type> _cmd_map;
        std::list<std::unique_ptr<command_base>> _active_cmd;
        ports_handler & _ports;
        
        public:

        command_server(ports_handler & l, port_type port) :
            service_base(l, port), _ports(l) {}

        void new_command(const std::string & name, factory_type factory)
        {
            _cmd_map.insert_or_assign(name, std::move(factory));
        }

        /* implements service_base::receive */
        void receive(packet p)
        {
            //FIXME this will call terminate if the data is corrupted or incorrect
            const jsoncons::json j = jsoncons::cbor::decode_cbor<jsoncons::json>(p.data());
            
            /* if the parsing fails, we do not reply */
            /* now we definitely will not reply since the thing will crash */

            /* check that the query contains the required keys */
            if (!j.is_object() || !j.contains("cmd") || !j["cmd"].is_string())
            {
                transmit_error(p, "object format");
                return;
            }
            if (!j.contains("args") || !j["args"].is_array())
            {
                transmit_error(p, "args format");
                return;
            }

            const auto & cmd = j["cmd"].as<std::string>();
            if (!_cmd_map.contains(cmd))
            {
                transmit_error(p, "command invalid: " + cmd);
                return;
            }

            // TODO implement the full spec including the separate service instance on a different port
            /* ports_handler.get_free_port() -> ports_handler.register_service() -> command_service_wrapper.bind_to()
            ports_handler needs to be able to unregister the service afterwards */
            
            /* invoke the registered factory, it will produce a new instance corresponding to this command name */
            auto cmd_inst = _cmd_map.at(cmd)();
            exit_status status;

            if (cmd_inst)
            {
                /* bind the command instance to the port handler and assign it a random free port */
                cmd_inst->bind_to(_ports);
                /* move the json object into the service to avoid any lifetime problems */
                status = cmd_inst->start(this, std::move(j));
            }
            else
                status = exit_status::ERR_RUNTIME; //FIXME?
            
            /* create a response informing of the command status */
            auto resp = p.create_response_packet();
            jsoncons::json jr (jsoncons::json_object_arg, {{"cmd", cmd}});
            //TODO add the optional id here

            switch (status)
            {
            case exit_status::DONE:
            case exit_status::CONTINUE:
                jr["port"] = (int)cmd_inst->get_port();
                break;
            
            case exit_status::ERR_ARGS:
                jr["err"] = "args invalid";
                break;

            case exit_status::ERR_RUNTIME:
                jr["err"] = "command runtime error";
                break;
            
            default:
                jr["err"] = "";
                break;
            }

            /* move the command instance into the active list if it returned CONTINUE
            //TODO such command will get checked on periodically in the main_task
            and eventually destroyed, just like the ones that ended straight away */
            if (status == exit_status::CONTINUE)
                _active_cmd.emplace_back(std::move(cmd_inst));
            /* need to null-check again in case we did not even construct the instance, 
            the above check works for that case naturally */
            else if (cmd_inst)
                _ports.unregister_service(cmd_inst->get_port());

            jsoncons::cbor::encode_cbor(jr, resp.data());
            transmit(std::move(resp));
        }

        private:

        /* create a response containing the error message and send it */
        void transmit_error(const packet & orig, std::string err)
        {
            auto resp = orig.create_response_packet();
            
            /* TODO replace string messages with error codes
            - the custom format could have a generic error type which has a field for an int and a string
            - exit_code and this error_code must be separate, exit_code is purely for commands to make
              the interface simple, the service needs to report more things */

            jsoncons::json jr (jsoncons::json_object_arg, {{"err", err}});
            jsoncons::cbor::encode_cbor(jr, resp.data());
            
            transmit(std::move(resp));
        }
    };
}


#endif



