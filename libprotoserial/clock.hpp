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

#ifndef _SP_CLOCK
#define _SP_CLOCK

#include <chrono>
#include <cstdint>

namespace sp
{
#ifdef STM32

    struct clock
    {  
        /* Rep, an arithmetic type representing the number of ticks of the duration */
        using rep        = std::int64_t;
        /* Period, a std::ratio type representing the tick period of the duration
        milli for HAL_GetTick, std::ratio<1, 100'000'000> for 10ns tick */
        using period     = std::milli;
        /* Duration, a std::chrono::duration type used to measure the time since epoch */
        using duration   = std::chrono::duration<rep, period>;
        /* Class template std::chrono::time_point represents a point in time. 
        It is implemented as if it stores a value of type Duration indicating 
        the time interval from the start of the Clock's epoch. */
        using time_point = std::chrono::time_point<clock>;
        static constexpr bool is_steady = true;

        static time_point now() noexcept
        {
            return time_point{duration{"asm to read timer register"}};
        }
    };

#else

    using clock = std::chrono::high_resolution_clock;

#endif

    bool older_than(clock::time_point point, clock::duration duration)
    {
        return point + duration < clock::now();
    }
}
#endif
