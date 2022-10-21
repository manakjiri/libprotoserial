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


#ifndef _SP_SERVICES_COMMON_COMMANDSTATUS
#define _SP_SERVICES_COMMON_COMMANDSTATUS


namespace sp
{
    enum class command_status //TODO
    {
        OK = 0,
        
        /* did not get past parsing group */
        /* request decode failed */
        REQUEST_FORMAT = 10,
        NAME_MISSING,
        ARGS_MISSING,
        PORT_MISSING,

        /* lookup failed group */
        NAME_INVALID = 20,
        ARGS_INVALID,

        /* runtime failed group */
        /* the factory function failed to produce the instance */
        NULL_INSTANCE = 30,
        /* command failed for some other reason than the arguments */
        COMMAND_RUNTIME,

        /* arguments group - this is the base code for argument errors
        40 means that there was a problem with the first argument
        42 means a problem with the third argument and so on */
        ARGUMENT_ERROR = 40,
    };
    
} // namespace sp

#endif
