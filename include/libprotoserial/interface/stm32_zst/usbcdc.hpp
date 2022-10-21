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

#ifndef _SP_INTERFACE_STM32ZST_USBCDC
#define _SP_INTERFACE_STM32ZST_USBCDC

#include "libprotoserial/interface/interface.hpp"
#include "libprotoserial/interface/parsers.hpp"

#include "zst/usb.hpp"

#include <cstring>
#include <atomic>

namespace sp
{
namespace detail
{
namespace stm32_zst
{
    template<class Header, class Footer>
    class usbcdc_interface : public interface
    {
		bytes _rx_buffer;
        zst::usb_cdc & _usb;
		uint _max_fragment_size;
		std::atomic<bool> _rx_buffer_lock;

        public:

        /* PACKET STRUCTURE: [Header][data >= 1][Footer] */

        /* - id should uniquely identify the USBCDC interface on this device
         * - address is the interface address, when a fragment is received where destination() == address
         *   then the receive_event is emitted, otherwise the other_receive_event is emitted
         * - max_queue_size sets the maximum number of fragments the transmit queue can hold
         */
        usbcdc_interface(zst::usb_cdc & usb, interface_identifier::instance_type instance, interface::address_type address,
            interface::address_type broadcast_address, uint max_queue_size, uint max_fragment_size) :
                interface(interface_identifier(interface_identifier::identifier_type::USBCDC, instance), address, broadcast_address,
                max_queue_size), _usb(usb), _max_fragment_size(max_fragment_size), _rx_buffer_lock(false)
        {
            _usb.receive_done.register_callback(&usbcdc_interface::isr_rx_done, this);
        }

        bytes::size_type max_data_size() const noexcept {return _max_fragment_size - (sizeof(Header) + sizeof(Footer) + 1);}
        prealloc_size minimum_prealloc() const noexcept {return prealloc_size(sizeof(Header) + 1, sizeof(Footer));}

        bool can_transmit() noexcept
		{
			return true; //TODO
		}

		bytes::size_type do_receive() noexcept
		{
			_rx_buffer_lock = true;
			bytes data = std::move(_rx_buffer);
			_rx_buffer_lock = false;

			if (data)
			{
                log_received_count(data.size());

                //TODO write linux interface that does not use the preamble
                for (int i = 0; data && data[0] == 0x55 && i < 3; i++)
                    data.shrink(1, 0);

                /* here we have the privilege of knowing the fragment size in advance, so
                all we need is a simple size check and then we simply copy everything out */
                if (data.size() > sizeof(Header) + sizeof(Footer))
                {
                    if (auto f = parsers::parse_fragment<Header, Footer>(std::move(data), *this))
                    {
                        put_received(std::move(*f));
                    }
                }
			}

			return 0;
		}

		bytes serialize_fragment(fragment && p) const
		{
			/* check if the data() has enough capacity */
			if (p.data().capacity_back() < sizeof(Footer) || p.data().capacity_front() < sizeof(Header) + 1)
				p.data().reserve(sizeof(Header) + 1, sizeof(Footer));
			
			/* Header */
			p.data().push_front(to_bytes(Header(p)));
			p.data().push_front(0x55);
			/* Footer */
			p.data().push_back(to_bytes(Footer(
				p.data().begin() + 1, p.data().end()
			)));

			/* move the data out of the packet and return it as an r-value,
			so it is obvious that we want to move it out of the function */
			return bytes(std::move(p.data()));
		}

		bool do_transmit(bytes && buff) noexcept
		{
			return _usb.transmit(static_cast<uint8_t*>(buff.data()), buff.size()).is_ok();
		}

		void isr_rx_done(uint8_t* data, uint32_t len)
		{
			if (!_rx_buffer_lock )
			{
				_rx_buffer = bytes(len);
				std::memcpy(_rx_buffer.data(), static_cast<byte*>(data), len);
			}
		}

    };
}
}
} // namespace sp

#endif
