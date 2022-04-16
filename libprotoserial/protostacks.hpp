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


#ifndef _SP_PROTOSTACKS
#define _SP_PROTOSTACKS

#include "libprotoserial/interface.hpp"
#include "libprotoserial/fragmentation.hpp"

#include <chrono>
using namespace std::chrono_literals;

namespace sp
{
namespace stack
{
    struct loopback
    {
        loopback_interface interface;
        fragmentation_handler fragmentation;

        loopback(sp::interface_identifier::instance_type instance, sp::interface::address_type address, uint rate,
            loopback_interface::transfer_function wire = [](byte b){return b;}):
                interface(instance, address, 255, 10, 64, 1024, wire), 
                fragmentation(interface, fragmentation_handler::configuration(interface, rate, interface.overhead_size(), 1024))
        {
            fragmentation.bind_to(interface);
        }

        void main_task()
        {
            interface.main_task();
            fragmentation.main_task();
        }
    };

    struct virtual_full
    {
        virtual_interface interface;
        fragmentation_handler fragmentation;

        virtual_full(sp::interface_identifier::instance_type instance, sp::interface::address_type address, uint rate) :
            interface(instance, address, 255, 10, 64, 1024), 
            fragmentation(interface, fragmentation_handler::configuration(interface, rate, interface.overhead_size(), 1024))
        {
            fragmentation.bind_to(interface);
        }

        void main_task()
        {
            interface.main_task();
            fragmentation.main_task();
        }
    };


/* #ifdef SP_UART
    struct uart_115200
    {
        uart_interface interface;
        fragmentation_handler fragmentation;

#if defined(SP_LINUX)
        uart_115200(std::string port, sp::interface_identifier::instance_type instance, sp::interface::address_type address):
            interface(port, B115200, instance, address, 255, 25, 64, 1024), 
#elif defined(SP_STM32)
        uart_115200(UART_HandleTypeDef * huart, interface_identifier::instance_type instance, interface::address_type address):
            interface(huart, instance, address, 255, 10, 64, 256), 
#endif
            fragmentation(interface.interface_id(), interface.max_data_size(), 25ms, 100ms, 3)
        {
            fragmentation.bind_to(interface);
        }

        void main_task()
        {
            interface.main_task();
            fragmentation.main_task();
        }

        void transfer_receive_subscribe(std::function<void(transfer)> fn)
        {
            fragmentation.transfer_receive_event.subscribe(fn);
        }

        void transfer_ack_subscribe(std::function<void(transfer_metadata)> fn)
        {
            fragmentation.transfer_ack_event.subscribe(fn);
        }

        void transfer_transmit(transfer tr)
        {
            fragmentation.transmit(std::move(tr));
        }

        interface_identifier interface_id() const
        {
            return interface.interface_id();
        }

    };
#endif */
}
}


#endif



