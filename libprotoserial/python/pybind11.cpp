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


#ifndef _SP_PYTHON_PROTOSTACKS
#define _SP_PYTHON_PROTOSTACKS

#define SP_FRAGMENTATION_DEBUG
#define SP_BUFFERED_DEBUG

#include "submodules/pybind11/include/pybind11/pybind11.h"
#include "submodules/pybind11/include/pybind11/functional.h"
#include "submodules/pybind11/include/pybind11/stl.h"

#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/protostacks.hpp"

namespace py = pybind11;


PYBIND11_MODULE(protoserial, m) {
    //py::class_<sp::bytes>(m, "bytes")

    py::class_<sp::interface_identifier>(m, "interface_identifier");

    py::class_<sp::transfer>(m, "transfer")
        .def(py::init<sp::interface_identifier, sp::transfer::id_type>())
        .def(py::init<sp::interface_identifier>())
        .def("data_contiguous", &sp::transfer::data_contiguous)
        .def("data_size", &sp::transfer::data_size)
        .def("push_back", static_cast<void(sp::transfer::*)(const sp::bytes &)>(&sp::transfer::push_back))
        .def("create_response", &sp::transfer::create_response)
        .def("set_destination", &sp::transfer::set_destination);

    py::class_<sp::stack::uart_115200>(m, "uart_115200")
        .def(py::init<std::string, sp::interface_identifier::instance_type, sp::interface::address_type>())
        .def("main_task", &sp::stack::uart_115200::main_task)
        .def("transfer_receive_subscribe", &sp::stack::uart_115200::transfer_receive_subscribe)
        .def("transfer_ack_subscribe", &sp::stack::uart_115200::transfer_ack_subscribe)
        .def("transfer_transmit", &sp::stack::uart_115200::transfer_transmit)
        .def("interface_id", &sp::stack::uart_115200::interface_id);
}

#endif



