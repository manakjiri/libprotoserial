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


#ifndef _SP_FRAGMENTATION_BYPASSHANDLER
#define _SP_FRAGMENTATION_BYPASSHANDLER

#include "libprotoserial/fragmentation/fragmentation.hpp"

namespace sp::detail
{
    class bypass_fragmentation_handler : public fragmentation_handler
    {
        public:
        
        bypass_fragmentation_handler(interface * i, prealloc_size prealloc) :
            fragmentation_handler(i, prealloc) {}

        protected:

        void do_receive(fragment f)
        {
            /* just wrap the fragment as if it was a transfer and pass it on */
            transfer_receive_event.emit(transfer(
                transfer_metadata(f, global_id_factory.new_id(f.interface_id())),
                std::move(f.data())
            ));
        }

        void do_main()
        {

        }
        
        void do_transmit(transfer t)
        {
            if (t.data().size() > _interface->max_data_size())
                transmit_complete_event.emit(t.object_id(), transmit_status::DROPPED);
            else
            {
                auto data = _prealloc.create(t.data().size());
                std::copy(t.data().begin(), t.data().end(), data.begin());
                transmit_event.emit(fragment(t.get_fragment_metadata(), std::move(data)));
            }
        }

        void transmit_began_callback(object_id_type id) 
        {
            //transmit_complete_event.emit(id, transmit_status::DONE);
        }

    };
}

#endif