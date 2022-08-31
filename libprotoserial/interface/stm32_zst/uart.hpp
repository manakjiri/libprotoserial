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

#ifndef _SP_INTERFACE_STM32ZST_UART
#define _SP_INTERFACE_STM32ZST_UART

#include "libprotoserial/interface/buffered.hpp"

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
    class uart_interface : public buffered_parser_interface<Header, Footer>
    {
        bytes _tx_buffer;
        zst::uart & _uart;
        std::atomic<bool> _is_transmitting;

        using parent = buffered_parser_interface<Header, Footer>;

        public:

        /* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

        /* - id should uniquely identify the UART interface on this device
         * - address is the interface address, when a fragment is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of fragments the transmit queue can hold
         * - buffer_size sets the size of the receive buffer in bytes
         */
        uart_interface(zst::uart & uart, interface_identifier::instance_type instance, interface::address_type address,
            interface::address_type broadcast_address, uint max_queue_size, uint max_fragment_size, uint buffer_size) :
                parent(interface_identifier(interface_identifier::identifier_type::UART, instance), address, broadcast_address,
                max_queue_size, buffer_size, max_fragment_size), _uart(uart), _is_transmitting(false)
        {
            _uart.receive_complete.register_callback(&uart_interface::isr_rx_done, this);
            _uart.transmit_complete.register_callback(&uart_interface::isr_tx_done, this);
            next_receive();
        }

        bool can_transmit() noexcept {return !_is_transmitting;} //FIXME

        protected:

        inline void isr_rx_done()
        {
            next_receive();
        }

        inline void isr_tx_done()
        {
            _is_transmitting = false;
        }

        inline void next_receive()
        {
            _uart.receive_it(reinterpret_cast<uint8_t*>(this->rx_buffer_future_write()), 1);
        }

        bool do_transmit(bytes && buff) noexcept
        {
            if (_is_transmitting)
                return false;

            _is_transmitting = true;
            _tx_buffer = std::move(buff);
            _uart.transmit_it(reinterpret_cast<uint8_t*>(_tx_buffer.data()), _tx_buffer.size());

            return true;
        }
    };
}
}
} // namespace sp

#endif
