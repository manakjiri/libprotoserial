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


#ifndef _SP_FRAGMENTATION_BASEHANDLER
#define _SP_FRAGMENTATION_BASEHANDLER

#include "libprotoserial/fragmentation/fragmentation.hpp"
#include "libprotoserial/fragmentation/transfer_handler.hpp"

namespace sp::detail
{
    template<typename Header>
    class base_fragmentation_handler : public fragmentation_handler
    {
        public:
        using message_types = typename Header::message_types;
        using fragmentation_handler::fragmentation_handler;

        protected:

        using transfer_handler_type = transfer_handler<Header>;
        using transfer_list_type = std::list<transfer_handler_type>;

        /* implementation of fragmentation_handler::do_receive */
        /* the callback handles the incoming fragments, it does not handle any timeouts, sending requests, 
        sending responses in general or anything that assumes periodicity, the main_task is for that */
        void do_receive(fragment f)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "receive got: " << f << std::endl;
#endif
            if (f && f.data().size() >= sizeof(Header))
            {
                /* copy the header from the fragment data after some obvious sanity checks */
                auto h = parsers::byte_copy<Header>(f.data().begin());
                if (h.is_valid())
                {
                    /* discard the header from fragment's data since we have it parsed out */
                    f.data().shrink(sizeof(Header), 0);

                    /* handle the reception of user fragments and their ordering */
                    //TODO break up using a switch and separate into functions 
                    if (h.type() == message_types::FRAGMENT)
                    {
                        if (f.data().size() > 0)
                        {
                            /* branch for handling incoming transfers */
                            /* check if we already know that incoming transfer ID */
                            if (auto ptr = find_transfer([&f, &h](const auto & tr){
                                return tr.is_incoming() && tr.is_part_of(f, h);}))
                            {
                                /* we know this ID, now we need to check if we have already received this transfer and
                                is therefor duplicate, or whether we are in the process of receiving it */
                                //TODO we need a way to remember already completed incoming transfers for some time to cover this
//                            if (it->has_data())
//                            {
#ifdef SP_FRAGMENTATION_DEBUG
                                    std::cout << "assigning to existing incoming transfer id " << (int)h.get_id() << " at " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                                    /* the ID is in known transfers, we need to add the incoming fragment to it */
                                    (*ptr)->put_fragment(h.fragment(), f);
//                            }
//                            else
//                            {
//                                /* we, for some reason, received a fragment of already received transfer, the ACK
//                                from us probably got lost in transit, just reply with another ACK and ignore this fragment */
//#ifdef SP_FRAGMENTATION_WARNING
//                                std::cout << "sending ACK for already received id " << (int)h.get_id() << std::endl;
//#endif
//                                transmit_event.emit(std::move(fragment(p.source(), 
//                                    std::move(to_bytes(make_header(types::FRAGMENT_ACK, h)))
//                                )));
//                            }
                            }
                            else
                            {
    #ifdef SP_FRAGMENTATION_DEBUG
                                std::cout << "creating new incoming transfer id " << (int)h.get_id() << std::endl;
    #endif
                                /* we don't know this transfer ID, create new incoming transfer */
                                _transfers.emplace_back(std::move(f), h);
                                //TODO limit the number of stored transfers
                            }
                        }
                    }
                    else if (h.type() == message_types::FRAGMENT_REQ)
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "handling retransmit request of id " << (int)h.get_id() << " fragment " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        if (auto ptr = find_transfer([&f, &h](const auto & tr){
                            return tr.is_outgoing() && tr.is_request_of(f, h);}))
                        {
                            /* when we get a retransmit request, we should queue it and actually handle it in 
                            the main task, where we also decide what has the highest priority,
                            this information can be stored in the transfer_handler using existing variables, 
                            since we only need one fragment of the transfer to be retransmitted */
                            if ((*ptr)->set_retransmit_request(h.fragment()))
                            {
                                //TODO retransmit set
                            }
                            else
                            {
                                //TODO retransmit set failed, request does not make sense
                            }
                        }
                        else
                        {
                            //TODO got retransmit request for a transfer we no longer have
                        }
                    }
                    else if (h.type() == message_types::FRAGMENT_ACK)
                    {
#ifdef SP_FRAGMENTATION_DEBUG
                        std::cout << "got fragment ACK for id " << (int)h.get_id() << std::endl;
#endif
                        if (auto ptr = find_transfer([&f, &h](const auto & tr){
                            return tr.is_outgoing() && tr.is_ack_of(f, h);}))
                        {
                            /* emit the ACK event for the sender and destroy this outgoing transfer
                            since we've done our job and don't need it anymore - in contrast to the 
                            incoming transfer where the transmitted ACK may not be received, here we
                            can be sure because if we receive another ACK for some reason, we'll 
                            just ignore it since we no longer store that transfer */
                            transmit_complete_event.emit((*ptr)->object_id(), transmit_status::DONE);
                            erase_transfer(*ptr);
                        }
                        else
                        {
                            //TODO ACK came for for a transfer we no longer have (ignore most likely)
                        }
                    }
                    else
                    {
                        //TODO unknown header message_type (ignore most likely)
                    }
                }
            }
        }
        
        /* implementation of fragmentation_handler::do_main */
        void do_main()
        {
            /* go through all active transfers and perform housekeeping
            - purge old/inactive/finished transfers
            - send ACKs and REQUESTs */
            auto ptr = _transfers.begin();
            while (ptr != _transfers.end())
            {
                //if (ptr->)
            }
        }
        
        /* implementation of fragmentation_handler::do_transmit */
        void do_transmit(transfer t)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "transmit got: " << t << std::endl;
#elif defined(SP_FRAGMENTATION_WARNING)
            std::cout << "transmit got id " << (int)t.get_id() << std::endl;
#endif
            //TODO limit the number of stored transfers
            _transfers.emplace_back(std::move(t), max_fragment_data_size());
        }

        /* implementation of fragmentation_handler::transmit_began_callback */
        void transmit_began_callback(object_id_type id)
        {


            
        }



        /* find first transfer in the internal transfer buffer that satisfies pred,  if not found */
        std::optional<typename transfer_list_type::iterator> find_transfer(std::function<bool(const transfer_handler_type &)> pred)
        {
            auto it = std::find_if(_transfers.begin(), _transfers.end(), pred);
            if (it != _transfers.end())
                return it;
            else
                return std::nullopt;
        }
        /* can only accept pointers returned by the find_transfer() function */
        void erase_transfer(transfer_list_type::iterator it)
        {
            _transfers.erase(it);
        }

        inline Header create_header(message_types type, index_type fragment_pos, const transfer_handler_type & t) const
        {
            return Header(type, fragment_pos, t.fragments_count(), t.get_id(), t.get_prev_id(), 0);
        }
        inline Header create_header(message_types type, const Header & h) const
        {
            return Header(type, h.fragment(), h.fragments_total(), h.get_id(), h.get_prev_id(), 0);
        }

        /* data size before the header is added */
        inline size_type max_fragment_data_size() const
        {
            /* _interface.max_data_size() is the maximum size of a fragment's data */
            return _interface.max_data_size() - sizeof(Header);
        }

        public:

/*         void print_debug() const
        {
#ifdef SP_ENABLE_IOSTREAM
            std::cout << "incoming_transfers: " << _incoming_transfers.size() << std::endl;
            for (const auto & t : _incoming_transfers)
                std::cout << static_cast<transfer>(t) << std::endl;
            
            std::cout << "outgoing_transfers: " << _outgoing_transfers.size() << std::endl;
            for (const auto & t : _outgoing_transfers)
                std::cout << static_cast<transfer>(t) << std::endl;
#endif
        } */

        private:

        transfer_list_type _transfers;
    };
}

#endif
