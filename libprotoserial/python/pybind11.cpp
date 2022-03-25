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

//#define SP_FRAGMENTATION_WARNING
//#define SP_BUFFERED_WARNING

#include "submodules/pybind11/include/pybind11/pybind11.h"
#include "submodules/pybind11/include/pybind11/functional.h"
#include "submodules/pybind11/include/pybind11/stl.h"

#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/protostacks.hpp"

#include <sstream>

namespace py = pybind11;



PYBIND11_MODULE(protoserial, m) {
    py::class_<sp::bytes>(m, "bytesbuff")
        .def(py::init([](py::bytes arg){
            return sp::bytes(std::string(arg));
        }))
        .def(py::init([](std::list<int> arg){
            sp::bytes ret(0, 0, arg.size());
            for (auto b : arg) ret.push_back((sp::byte)b);
            return ret;
        }))
        .def("__repr__", [](const sp::bytes &a) {
            std::stringstream s; s << "bytesbuff(" << a << ')';
            return s.str();
        })
        .def("as_list", [](const sp::bytes &arg) {
            std::list<int> ret;
            for (auto b : arg) ret.push_back((int)b);
            return ret;
        });

    py::class_<sp::interface_identifier>(m, "interface_identifier")
        .def("__repr__", [](const sp::interface_identifier &a) {
            std::stringstream s; s << "interface_identifier(" << a << ')';
            return s.str();
        });
    
    py::class_<sp::transfer_metadata>(m, "transfer_metadata")
        .def("get_id", &sp::transfer_metadata::get_id)
        .def("get_prev_id", &sp::transfer_metadata::get_prev_id)
        .def("source", &sp::transfer_metadata::source)
        .def("destination", &sp::transfer_metadata::destination)
        .def("interface_id", &sp::transfer_metadata::interface_id);

    py::class_<sp::transfer, sp::transfer_metadata>(m, "transfer")
        .def(py::init<sp::interface_identifier, sp::transfer::id_type>())
        .def(py::init<sp::interface_identifier>())
        .def(py::init<const sp::interface &, sp::transfer::id_type>())
        .def(py::init<const sp::interface &>())
        .def("data", &sp::transfer::data_contiguous)
        .def("data_size", &sp::transfer::data_size)
        .def("push_back", static_cast<void(sp::transfer::*)(const sp::bytes &)>(&sp::transfer::push_back))
        //.def("push_back", [](py::bytes arg){return sp::bytes(std::string(arg));})
        .def("create_response", &sp::transfer::create_response)
        .def("set_destination", &sp::transfer::set_destination)
        /* inherited from sp::transfer_metadata */
        .def("get_id", static_cast<sp::transfer::id_type(sp::transfer::*)() const>(&sp::transfer::get_id))
        .def("get_prev_id", static_cast<sp::transfer::id_type(sp::transfer::*)() const>(&sp::transfer::get_prev_id))
        .def("source", static_cast<sp::transfer::address_type(sp::transfer::*)() const>(&sp::transfer::source))
        .def("destination", static_cast<sp::transfer::address_type(sp::transfer::*)() const>(&sp::transfer::destination))
        .def("interface_id", static_cast<sp::interface_identifier(sp::transfer::*)() const>(&sp::transfer::interface_id))
        /* additional */
        .def("__repr__", [](const sp::transfer &a) {
            std::stringstream s; s << "transfer(" << a << ')';
            return s.str();
        });

    py::class_<sp::stack::uart_115200>(m, "uart_115200")
        .def(py::init<std::string, sp::interface_identifier::instance_type, sp::interface::address_type>())
        .def("main_task", &sp::stack::uart_115200::main_task)
        .def("transfer_receive_subscribe", &sp::stack::uart_115200::transfer_receive_subscribe)
        .def("transfer_ack_subscribe", &sp::stack::uart_115200::transfer_ack_subscribe)
        .def("transfer_transmit", &sp::stack::uart_115200::transfer_transmit)
        .def("interface_id", &sp::stack::uart_115200::interface_id)
        .def("new_transfer", [](const sp::stack::uart_115200 & arg){
            return sp::transfer(arg.interface);
        });
}

#endif



