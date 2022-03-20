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

#ifndef _SP_INTERFACE_LINUX_UART
#define _SP_INTERFACE_LINUX_UART

#include "libprotoserial/interface/buffered_parsed.hpp"

#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include <stdexcept>

namespace sp
{
namespace detail
{
namespace linux
{
template<class Header, class Footer>
class uart_interface : public buffered_parsed_interface<Header, Footer>
{
    using parent = buffered_parsed_interface<Header, Footer>;
    
    public: 

    struct open_failed : std::exception {
        open_failed(std::string m): _m(std::move(m)) {}
        const char* what () const throw () {return _m.c_str();}
        std::string _m;
    };

    uart_interface(std::string port, speed_t baud, interface_identifier::instance_type instance, interface::address_type address, 
        uint max_queue_size, uint max_fragment_size, uint buffer_size):
            parent(interface_identifier(interface_identifier::identifier_type::UART, instance), address, max_queue_size, buffer_size, max_fragment_size)
    {
        // Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
        int _serial_port = open(port.c_str(), O_RDWR);

        // Create new termios struct, we call it 'tty' for convention
        struct termios tty;

        // Read in existing settings, and handle any error
        if(tcgetattr(_serial_port, &tty) != 0) 
            throw open_failed("Error " + std::to_string(errno) + " from tcgetattr: " + strerror(errno));

        tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
        tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
        tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
        tty.c_cflag |= CS8; // 8 bits per byte (most common)
        tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
        tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO; // Disable echo
        tty.c_lflag &= ~ECHOE; // Disable erasure
        tty.c_lflag &= ~ECHONL; // Disable new-line echo
        tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

        tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
        tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
        // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
        // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

        tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
        tty.c_cc[VMIN] = 0;

        // Set in/out baud rate to be 9600
        cfsetispeed(&tty, baud);
        cfsetospeed(&tty, baud);

        // Save tty settings, also checking for error
        if (tcsetattr(_serial_port, TCSANOW, &tty) != 0)
            throw open_failed("Error " + std::to_string(errno) + " from tcgetattr: " + strerror(errno));
    }

    ~uart_interface() 
    {
        close(_serial_port);
    }

    protected:

    bool can_transmit() noexcept {return true;}
    bool do_transmit(bytes && buff) noexcept 
    {
        write(_serial_port, buff.data(), buff.size());
        return true;
    }
    void do_single_receive() 
    {
        uint8_t read_buf;
        int num_bytes = read(_serial_port, &read_buf, 1);
        if (num_bytes > 0)
            this->put_single_received((byte)read_buf);
    }

    int _serial_port;
};
}
}
} // namespace sp

#endif
