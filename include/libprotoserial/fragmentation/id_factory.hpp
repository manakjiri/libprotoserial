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

#ifndef _SP_FRAGMENTATION_ID_FACTORY
#define _SP_FRAGMENTATION_ID_FACTORY

#include "libprotoserial/interface/interface_id.hpp"

#include <list>
#include <algorithm>

namespace sp
{

    struct id_factory
    {
        using id_type = uint;
        
        private:
        struct mapper
        {
            interface_identifier interface_id;
            id_type id_count;

            mapper(interface_identifier iid) : 
                interface_id(iid), id_count(0) {}
        };

        std::list<mapper> _mappers;

        id_type increment_id(id_type & id_counter)
        {
            ++id_counter;
            if ((std::uint8_t)id_counter == 0) ++id_counter;
            return id_counter;
        }

        public:

        id_type new_id(interface_identifier iid)
        {
            auto mit = std::find_if(_mappers.begin(), _mappers.end(), [&](const auto & mp){
                return mp.interface_id == iid;
            });
            if (mit == _mappers.end())
            {
                auto & m = _mappers.emplace_back(iid);
                return increment_id(m.id_count);
            }
            else
                return increment_id(mit->id_count);
        }
    };

    /* ID factory that issues transfer IDs based on the interface, as per spec */
    static id_factory global_id_factory;
}


#endif