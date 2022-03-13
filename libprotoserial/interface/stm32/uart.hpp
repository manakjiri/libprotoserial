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

#include "libprotoserial/libconfig.hpp"

#include "libprotoserial/interface/buffered.hpp"
#include "libprotoserial/interface/parsers.hpp"

#include <memory>

namespace sp
{
namespace detail
{
namespace stm32
{
	template<class Header, class Footer>
	class uart_interface : public buffered_interface
	{
		public:

		typedef sp::byte preamble_type;
		const preamble_type preamble = (preamble_type)0x55;
		const typename Header::size_type preamble_length = 2;

		/* PACKET STRUCTURE: [preamble][preamble][Header][data >= 1][Footer] */

		/* - id should uniquely identify the UART interface on this device
		 * - address is the interface address, when a packet is received where destination() == address
		 *   then the receive_event is emitted, otherwise the other_receive_event is emitted
		 * - max_queue_size sets the maximum number of packets the transmit queue can hold
		 * - buffer_size sets the size of the receive buffer in bytes
		 */
		uart_interface(UART_HandleTypeDef * huart, uint id, interface::address_type address, uint max_queue_size,
				uint max_packet_size, uint buffer_size) :
			buffered_interface("uart" + std::to_string(id), address, max_queue_size, buffer_size), _huart(huart),
			_max_packet_size(max_packet_size), _is_transmitting(false)
		{
			_read = get_rx_buffer();
			_write = get_rx_buffer();
			next_receive();
		}

		bytes::size_type max_data_size() const noexcept {return _max_packet_size - sizeof(Header) - sizeof(Footer) - preamble_length;}
		bool can_transmit() noexcept {return _is_transmitting;}

		void isr_rx_done()
		{
			/* the iterator wraps around to the beginning at the end of the buffer */
			next_receive();
		}

		void isr_tx_done()
		{
			_is_transmitting = false;
		}

		protected:

		inline void next_receive()
		{
			HAL_UART_Receive_IT(_huart, reinterpret_cast<uint8_t*>(&(*(++_write))), 1);
		}

		void do_receive() noexcept
		{
			/* while we are trying to parse the buffer, the ISR is continually filling it
			(not in this case, obviously, but in the real world it will)
			_write is the position of the last byte written, so we can read up to that point
			since this way we cannot possibly collide with the ISR */
			auto read = _read;
			auto write = _write;

			/* while is necessary since we would never move forward in case we find a valid preamble but fail
			before the parsing */
			while (read != write)
			{
				/* if this returns false than read == write and we break out of the while loop */
				if (parsers::find(read, write, preamble))
				{
					/* read now points to the position of the preamble, we are no longer concerned with the preamble */
					auto packet_start = read + 1;
					/* check if the Header is already loaded into to buffer, if not this function will just return
					and try new time around, we cannot move the original _read just yet because of this,
					adding 1 to distance since we can also read the write byte */
					if ((size_t)distance(packet_start, write) + 1 >= sizeof(Header))
					{
						/* copy the Header into the Header structure byte by byte */
						Header h = parsers::byte_copy<Header>(packet_start, packet_start + sizeof(Header));
						if (h.is_valid(max_data_size()))
						{
							/* total packet size */
							size_t packet_size = h.size + sizeof(Footer) + sizeof(Header);
							/* once again, check that there are enough bytes in the buffer, this can still fail */
							if ((size_t)distance(packet_start, write) + 1 >= packet_size)
							{
								/* we have received the entire packet, prepare it for parsing */
								//bytes b(packet_size);
								//std::copy(packet_start, packet_start + packet_size, b.begin());
								auto b = parsers::byte_copy(packet_start, packet_start + packet_size);
								try
								{
									/* attempt the parsing */
									put_received(std::move(parsers::parse_packet<Header, Footer>(std::move(b), this)));
									/* parsing succeeded, finally move the read pointer, we do not include the
									preamble length here because we don't necessarily know how long it was originally */
									_read = read + packet_size;
								}
								catch(std::exception &e)
								{
									/* parsing failed, move by one because there is no need to try and parse this again */
									_read = read + 1;
								}
								return;
							}
							else
							{
								/* once again we just failed the distance check for the whole packet this time,
								we cannot save the read position because the Header check could be wrong as well */
								_read = read;
								return;
							}
						}
						else
						{
							/* we failed the size valid check, so this is either a corrupted Header or it's not a Header
							at all, move the read pointer past this and try again */
							_read = read = packet_start;
						}
					}
					else
					{
						/* we failed the distance check for the Header, we need to wait for the buffer to fill some more,
						store the current read position as the global _read so that find returns faster once we come back */
						_read = read;
						return;
					}
				}
			}
		}

		bytes serialize_packet(packet && p) const
		{
			/* preallocate the container since we know the final size */
			auto b = bytes(0, 0, preamble_length + sizeof(Header) + p.data().size() + sizeof(Footer));
			/* preamble */
			auto pr = bytes(preamble_length);
			pr.set(preamble);
			b.push_back(pr);
			/* Header */
			b.push_back(to_bytes(Header(p)));
			/* data */
			b.push_back(p.data());
			/* Footer */
			b.push_back(to_bytes(Footer(b.begin() + preamble_length, b.end())));
			return b;
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
		buffered_interface::circular_iterator _write;
		buffered_interface::circular_iterator _read;
		bytes _tx_buffer;
		UART_HandleTypeDef * _huart;
		uint _max_packet_size;
		volatile bool _is_transmitting;
	};
}
}
} // namespace sp

#endif
