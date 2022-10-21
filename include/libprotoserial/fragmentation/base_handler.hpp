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

#include <list>
#include <functional>

#include "libprotoserial/fragmentation/fragmentation.hpp"
#include "libprotoserial/fragmentation/transfer_handler.hpp"


namespace sp
{
namespace detail
{
    //template<class Header>
    class base_fragmentation_handler : public fragmentation_handler
    {
        public:
        using Header = headers::fragment_8b8b;
        using message_types = typename Header::message_types;
        using fragmentation_handler::fragmentation_handler;

        protected:

        using transfer_handler_type = transfer_handler<Header>;
        using transfer_list_type = std::list<transfer_handler_type>;

        transfer_list_type transfers;

        
        /* calculate priority score of the transfer, transfer with highest score is selected for transmit */
        virtual int transfer_transmit_priority(const transfer_handler_type &) = 0;
        /* this function is usually called before transfer_transmit_priority(), it disqualifies transfers
        targeted at peers that risk overload */
        virtual bool is_peer_ready_to_receive_data_fragment(const transfer_handler_type &) = 0;
        /* this function is called to check whether it is desirable to initiate a data fragment transmit 
        of some transfer (usually the one that has highest priority, see function prototype above). implement 
        this function to prevent interface overload. note that this function returning true does not guarantee 
        a fragment transmit */
        virtual bool is_fragment_transmit_allowed() = 0;
        /* this function is called when a response to bordering fragment of the transfer was received */
        virtual void bordering_fragment_response_received(const transfer_handler_type &, message_types) = 0;


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
                            auto itr = find_transfer([&f, &h](const auto & tr){return tr.is_incoming() && tr.is_part_of(f, h);});
                            if (itr != transfers.end())
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
                                    itr->put_fragment(h.fragment(), f);
                                    //TODO if we receive the first fragment multiple times, we were slow to ACK it, the ACK got lost or the source is too fast
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
                                /* we don't know this transfer ID, attempt to create new incoming transfer.
                                we should always receive the first fragment of a transfer first, because we need to ACK
                                it before the transfer can continue */
                                //TODO limit the number of stored transfers
                                if (h.fragment() == 1)
                                {
                                    transfers.emplace_back(std::move(f), h);
                                }
                            }
                        }
                    }
                    else if (h.type() == message_types::FRAGMENT_REQ)
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "handling retransmit request of id " << (int)h.get_id() << " fragment " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        auto itr = find_transfer([&f, &h](const auto & tr){return tr.is_outgoing() && tr.is_request_of(f, h);});
                        if (itr != transfers.end())
                        {
                            /* store the measurement of the round trip time */
                            if (itr->is_current_bordering())
                                bordering_fragment_response_received(*itr, h.type());

                            /* when we get a retransmit request, we should queue it and actually handle it in 
                            the main task, where we also decide what has the highest priority,
                            this information can be stored in the transfer_handler using existing variables, 
                            since we only need one fragment of the transfer to be retransmitted */
                            if (itr->retransmit_request(h.fragment()))
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
                        auto itr = find_transfer([&f, &h](const auto & tr){return tr.is_outgoing() && tr.is_ack_of(f, h);});
                        if (itr != transfers.end())
                        {
                            /* store the measurement of the round trip time */
                            if (itr->is_current_bordering())
                                bordering_fragment_response_received(*itr, h.type());

                            /* emit the ACK event for the sender and destroy this outgoing transfer
                            since we've done our job and don't need it anymore - in contrast to the 
                            incoming transfer where the transmitted ACK may not be received, here we
                            can be sure because if we receive another ACK for some reason, we'll 
                            just ignore it since we no longer store that transfer */
                            transmit_complete_event.emit(itr->object_id(), transmit_status::DONE);
                            transfers.erase(itr);
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
            auto itr = transfers.begin();
            while (itr != transfers.end())
            {
                if (itr->is_outgoing())
                {
                    
                    
                }
                else if (itr->is_incoming())
                {
                    /*  */
                }
            }

            if (!is_fragment_transmit_allowed())
                return;

            /* select transfer with highest priority */
            transfer_handler_type * to_transmit = nullptr;
            for (auto & t : transfers)
            {
                if (t.is_outgoing() && t.is_transmit_ready() && is_peer_ready_to_receive_data_fragment(t))
                {
                    /* select the first one to pass the above, if there are more, select the max */
                    if (!to_transmit)
                        to_transmit = &t;
                    else if(transfer_transmit_priority(*to_transmit) < transfer_transmit_priority(t))
                        to_transmit = &t;
                }
            }
            
            if (to_transmit)
            {
                if (auto f = to_transmit->get_current_fragment(_prealloc))
                {
                    transmit_fragment(std::move(*f));
                }
                else
                {
                    //TODO get_current_fragment failed - internal handling error
                }
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
            transfers.emplace_back(std::move(t), max_fragment_data_size());
        }

        /* implementation of fragmentation_handler::transmit_began_callback */
        void transmit_began_callback(object_id_type id)
        {
            auto ptr = find_transfer([&id](const auto & tr){return tr.is_fragment_transmitted_of(id);});
            if (ptr != transfers.end())
            {
                /* start measuring Round Trip Time */
                if (!ptr->fragment_transmitted())
                {
                    //TODO some transfer state error
                }
            }
            else
            {
                //TODO transmit_began_callback of fragment we no longer have
            }
        }



        /* find first transfer in the internal transfer buffer that satisfies pred */
        inline typename transfer_list_type::iterator find_transfer(std::function<bool(const transfer_handler_type &)> pred)
        {
            return std::find_if(transfers.begin(), transfers.end(), pred);
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

        void transmit_fragment(fragment f)
        {
            transmit_event.emit(std::move(f));
        }

        public:

/*         void print_debug() const
        {
#ifdef SP_ENABLE_IOSTREAM
            std::cout << "incomingtransfers: " << _incomingtransfers.size() << std::endl;
            for (const auto & t : _incomingtransfers)
                std::cout << static_cast<transfer>(t) << std::endl;
            
            std::cout << "outgoingtransfers: " << _outgoingtransfers.size() << std::endl;
            for (const auto & t : _outgoingtransfers)
                std::cout << static_cast<transfer>(t) << std::endl;
#endif
        } */        
    };
}
}

#endif
