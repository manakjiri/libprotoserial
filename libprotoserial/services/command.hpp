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


#ifndef _SP_SERVICES_COMMAND
#define _SP_SERVICES_COMMAND

#include <libprotoserial/services/service_base.hpp>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>

#include <string>
#include <unordered_map>


namespace sp
{
    class command_service : public service_base
    {
        public:

        enum class exit_status { DONE, CONTINUE, ERR_ARGS, ERR_RUNTIME };

        class command_args
        {
            jsoncons::json & _args;

            public:
            command_args(jsoncons::json & args) :
                _args(args) {}

            /* get the n'th argument of specified type, starting from arg0, 
            allowed types are int, double, bool, std::string, sp::bytes
            you will get a build error if these types are not used */
            template<typename ArgType> 
            std::optional<ArgType> get(const std::size_t n) const
            {
                /* presence of the "args" key is checked in the service already */
                const auto & args = _args["args"];

                /* "args" should be an array */
                if (!args.is_array())
                    return std::nullopt;

                /* it should contain the index the user is asking for */
                if (n >= args.size())
                    return std::nullopt;
                
                const auto & arg = args[n];
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

        class command_base
        {
            jsoncons::json _args;
            command_service & _service;

            public:
            using enum exit_status;

            command_base(command_service & service) :
                _service(service) {}

            exit_status start(jsoncons::json args)
            {
                _args = std::move(args);
                return run(command_args(_args));
            }

            protected:

            virtual exit_status run(const command_args & args) = 0;
        };

        private:
        
        std::unordered_map<std::string, std::unique_ptr<command_base>> _cmd_map;
        
        public:

        command_service(ports_handler & l, port_type port) :
            service_base(l, port) {}

        /* returns a pointer to a newly constructed instance of the CommandClass, 
        returns nullptr if an instance with the same name already exists */
        template<class CommandClass, typename... CommandArgs>
        constexpr CommandClass * new_command(const std::string & name, CommandArgs... command_args)
        {
            static_assert(std::is_base_of<command_base, CommandClass>::value, "CommandClass needs to derive from command_base");
            CommandClass * ret = nullptr;

            if (!_cmd_map.contains(name))
            {
                ret = new CommandClass(*this, std::forward<CommandArgs>(command_args)...);
                _cmd_map.insert({name, std::unique_ptr<command_base>(static_cast<command_base*>(ret))});
            }

            return ret;
        }

        /* implements service_base::receive */
        void receive(packet p)
        {
            //jsoncons::json j = cbor::decode_cbor<json>(data);
        }
    };
}


#endif



