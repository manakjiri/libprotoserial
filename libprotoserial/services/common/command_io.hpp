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


#ifndef _SP_SERVICES_COMMON_COMMANDIO
#define _SP_SERVICES_COMMON_COMMANDIO

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/jsonpath/jsonpath.hpp>


namespace sp
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

#endif
