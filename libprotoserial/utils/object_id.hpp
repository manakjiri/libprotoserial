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

#include "libprotoserial/libconfig.hpp"
#include "libprotoserial/data/container.hpp"

namespace sp
{
    using object_id_type = uint;
    
    /* this class is suitable as a base class, its purpose is to provide trackability of objects 
    with minimum overhead. It is mainly used to track fragments, transfers and packets. */
    struct sp_object
    {
        sp_object() : sp_object(_new_id()) {}

        sp_object(const sp_object &) : sp_object() {}
        sp_object(sp_object && obj) : sp_object(obj.object_id()) {}
        sp_object & operator=(const sp_object &) {_id = _new_id(); return *this;}
        sp_object & operator=(sp_object && obj) {_id = obj.object_id(); return *this;}

        /* this ID is tracked globally and should be unique on a modest
        time scale. All ID values are valid. Move preserves the ID but a Copy
        will generate a new ID for the copy of the original object */
        const object_id_type object_id() const noexcept {return _id;}

        private:

        sp_object(object_id_type id) : _id(id) {}
        
        object_id_type _new_id() 
        {
            static object_id_type _id_count = 0;
            return ++_id_count;
        }

        object_id_type _id;
    };
}


