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

#ifndef _SP_INTERFACE_INTERFACE_ID
#define _SP_INTERFACE_INTERFACE_ID

#include <cinttypes>

#ifndef SP_NO_IOSTREAM
#include <iostream>
#endif

namespace sp
{
    struct interface_identifier
    {
        using instance_type = std::uint8_t;
        enum identifier_type : std::uint8_t
        {
            NONE,
            VIRTUAL,
            LOOPBACK,
            UART,
        };

        constexpr interface_identifier(identifier_type id, instance_type inst) :
            identifier(id), instance(inst) {}

        constexpr interface_identifier() :
            interface_identifier(identifier_type::NONE, 0) {}

        constexpr interface_identifier(const interface_identifier &) = default;
        constexpr interface_identifier(interface_identifier &&) = default;
        constexpr interface_identifier & operator=(const interface_identifier &) = default;
        constexpr interface_identifier & operator=(interface_identifier &&) = default;
        
        constexpr bool operator==(const interface_identifier &) const = default;
        constexpr bool operator!=(const interface_identifier &) const = default;

        identifier_type identifier;
        instance_type instance;
    };   
}

#ifndef SP_NO_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::interface_identifier & iid) 
{
    os << (int)iid.identifier << '-' << (int)iid.instance;
    return os;
}
#endif

#endif

