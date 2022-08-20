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

#ifndef _SP_BITRATE
#define _SP_BITRATE

#include "libprotoserial/libconfig.hpp"
#include "libprotoserial/clock.hpp"

namespace sp
{
    /* class for representing data rate in bits per second (an unsigned integer) */
    class bit_rate
    {
        public:
        using unit_type = uint_least32_t;

        constexpr bit_rate(unit_type rate) noexcept :
            _rate(rate) {}

        constexpr bit_rate() noexcept :
            bit_rate(0) {}

        constexpr bit_rate(const bit_rate&) = default;
        constexpr bit_rate(bit_rate&&) = default;
        constexpr bit_rate& operator=(const bit_rate&) = default;
        constexpr bit_rate& operator=(bit_rate&&) = default;

        constexpr operator unit_type() const noexcept
        {
            return _rate;
        }

        /* calculates one bit time from the bit rate (frequency) */
        constexpr clock::duration bit_period() const noexcept
        {
            /* _rate represent bits per second, we need (1s / _rate) to get the period
            clock::duration is some large integer, usually representing nanosecond reps
            so we can just force everything to nanoseconds and go from there */
            return clock::duration{std::chrono::nanoseconds{1'000'000'000} / std::chrono::nanoseconds{_rate}};
        }

        private:
        unit_type _rate;
    };
}

#endif
