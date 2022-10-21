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

#ifndef _SP_INTERFACE_STM32_UART
#define _SP_INTERFACE_STM32_UART

#include "libprotoserial/interface/buffered.hpp"

#include <memory>

namespace sp
{
namespace detail
{
namespace stm32
{
	template<class Header, class Footer>
	class uart_interface : public buffered_parser_interface<Header, Footer>
	{
		using parent = buffered_parser_interface<Header, Footer>;
		
		public:

		/* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

		/* - id should uniquely identify the UART interface on this device
		 * - address is the interface address, when a fragment is received where destination() == address
		 *   then the receive_event is emitted, otherwise the other_receive_event is emitted
		 * - max_queue_size sets the maximum number of fragments the transmit queue can hold
		 * - buffer_size sets the size of the receive buffer in bytes
		 */
		uart_interface(UART_HandleTypeDef * huart, interface_identifier::instance_type instance, interface::address_type address, 
			interface::address_type broadcast_address, uint max_queue_size, uint max_fragment_size, uint buffer_size) :
				parent(interface_identifier(interface_identifier::identifier_type::UART, instance), address, broadcast_address, 
				max_queue_size, buffer_size, max_fragment_size), _huart(huart), _is_transmitting(false)
		{
			next_receive();
		}

		bool can_transmit() noexcept {return !_is_transmitting;}

		inline volatile void isr_rx_done()
		{
			next_receive();
		}

		inline volatile void isr_tx_done()
		{
			_is_transmitting = false;
		}

		protected:

		inline void next_receive()
		{
			HAL_UART_Receive_IT(_huart, reinterpret_cast<uint8_t*>(this->rx_buffer_future_write()), 1);
			HAL_GPIO_TogglePin(DBG1_GPIO_Port, DBG1_Pin);
		}

		bool do_transmit(bytes && buff) noexcept
		{
			if (_is_transmitting)
				return false;

			_is_transmitting = true;
			_tx_buffer = std::move(buff);
			HAL_UART_Transmit_IT(_huart, reinterpret_cast<uint8_t*>(_tx_buffer.data()),
					_tx_buffer.size());

			return true;
		}

		private:
		bytes _tx_buffer;
		UART_HandleTypeDef * _huart;
		volatile bool _is_transmitting;
	};
}
}
} // namespace sp

#endif
