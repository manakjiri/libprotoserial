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


#ifndef _SP_FRAGMENTATION
#define _SP_FRAGMENTATION


#include <libprotoserial/fragmentation/fragmentation.hpp>
#include <libprotoserial/fragmentation/bypass_handler.hpp>
#include <libprotoserial/fragmentation/base_handler.hpp>


namespace sp
{
    class bypass_fragmentation_handler : public detail::bypass_fragmentation_handler
    {
        using detail::bypass_fragmentation_handler::bypass_fragmentation_handler;
    };
}


#endif

