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

#ifndef _SP_INTERFACE_STM32ZST_RS485
#define _SP_INTERFACE_STM32ZST_RS485

#include "libprotoserial/interface/stm32_zst/uart.hpp"

#include "zst/gpio.hpp"
#include "zst/uart.hpp"

#include <memory>
#include <atomic>

namespace sp
{
namespace detail
{
namespace stm32_zst
{
    template<class Header, class Footer>
    class rs485_interface : public uart_interface<Header, Footer>
    {
        using parent = uart_interface<Header, Footer>;

        zst::gpio _de_pin, _nre_pin;

        public:

        /* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

        /* - id should uniquely identify the UART interface on this device
         * - address is the interface address, when a fragment is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of fragments the transmit queue can hold
         * - buffer_size sets the size of the receive buffer in bytes
         */
        rs485_interface(zst::uart & uart, zst::gpio de_pin, zst::gpio nre_pin, interface_identifier::instance_type instance, interface::address_type address,
            interface::address_type broadcast_address, uint max_queue_size, uint max_fragment_size, uint buffer_size) :
                parent(uart, instance, address, broadcast_address, max_queue_size, buffer_size, max_fragment_size), _de_pin(de_pin), _nre_pin(nre_pin)
        {
            /* drive Receiver Output low to enable data receive */
            _nre_pin.reset();
            /* drive Data Enable low since we are not transmitting */
            _de_pin.reset();
        }

        void isr_tx_done()
        {
            parent::isr_tx_done();
            _nre_pin.reset();
            _de_pin.reset();
        }

        bool do_transmit(bytes && buff) noexcept
        {
            if (parent::do_transmit(std::move(buff)))
            {
                _nre_pin.set();
                _de_pin.set();
                return true;
            }
            return false;
        }

    };
}
}
} // namespace sp

#endif
