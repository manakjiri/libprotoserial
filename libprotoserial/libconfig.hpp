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

#ifndef _SP_LIBCONFIG
#define _SP_LIBCONFIG


#ifdef STM32L0
#include "stm32l0xx_hal.h"
#define SP_STM32
#endif

#ifdef SP_STM32
#define SP_EMBEDDED
#endif

#ifdef ZST
#define SP_STM32ZST
#define SP_EMBEDDED
#endif

#ifdef __linux__
#define SP_LINUX
#endif

#ifdef SP_LINUX
#define SP_ENABLE_IOSTREAM
#define SP_ENABLE_EXCEPTIONS
#endif

#ifdef SP_ENABLE_IOSTREAM
#if __has_include(<format>)
#include <format>
#define SP_ENABLE_FORMAT
#else
#include <iomanip>
#endif
#endif

namespace sp
{
#ifdef SP_ENABLE_FORMAT
    using std::format;
#endif
}

#endif

