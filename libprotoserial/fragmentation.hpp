/* 
 * > We can build the packet fragmentation logic on top of the interface, 
 * > this logic should have its own internal buffer for packets received
 * > from events, because once the event firess, the packet is forgotten on 
 * > the interface side to avoid the need for direct access to the interface's 
 * > RX queue.
 * 
 * 
 * 
 * 
 */




#ifndef _SP_FRAGMENTATION
#define _SP_FRAGMENTATION

#include "libprotoserial/interface.hpp"
#include "libprotoserial/transfer.hpp"
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/parsers.hpp"
#include "libprotoserial/clock.hpp"

namespace sp
{
    class fragmentation_handler
    {
        public:

        using header = headers::fragment_header_8b16b;
        using types = header::message_types;
        using index_type = typename header::index_type;
        
        /* we'd like to represent something like 115200bps, we could use clock::rep's per bit but
        that would restrict us when the bit rate is higher than the ticks -> use fraction? */
        //using bitrate_type = 
        
        fragmentation_handler(clock::duration retransmit_time, clock::duration drop_time) :
            _retransmit_time(retransmit_time), _drop_time(drop_time) {}


        /* the callback handles the incoming packets, it does not handle any timeouts, sending requests, 
        or anything that assumes periodicity, the main_task is for that */
        void receive_callback(interface::packet p) noexcept
        {
            if (p && p.data().size() >= sizeof(header))
            {
                /* copy the header from the packet data after some obvious sanity checks, discard 
                the header from packet's data afterward */
                auto h = parsers::byte_copy<header>(p.data().begin(), p.data().begin() + sizeof(header));
                if (h.is_valid())
                {
                    p.data().shrink(sizeof(header), 0);
                    handle_packet(h, std::move(p));
                }
            }
        }

        void main_task()
        {
            auto tr = _incoming_transfers.begin();
            while (tr != _incoming_transfers.end())
            {
                if (tr->is_complete())
                {
                    /* checks whether any of the transfers is complete, if so emits 
                    the receive_event and removes that transfer from _incoming_transfers */
                    transfer_receive_event.emit(std::move(*tr));
                    tr = _incoming_transfers.erase(tr);
                }
                else
                {
                    //TODO timeout, retransmit request atd
                    if (older_than(tr->timestamp_newest(), _drop_time))
                    {
                        /* drop the incoming transfer because it is inactive for too long */
                        tr = _incoming_transfers.erase(tr);
                    }
                    else if (older_than(tr->timestamp_newest(), _retransmit_time))
                    {
                        /* find the missing packet's index and request a retransmit */
                        auto index = tr->missing_fragment();
                        transmit_event.emit(
                            interface::packet(tr->source(), 
                            to_bytes(make_header(types::PACKET_REQ, index, *tr)))
                        );
                    }
                    /* check again because an erase can happen in this branch as well, 
                    we don't need to worry about skipping a transfer when the erase happens
                    since we will return into this function later anyway */
                    if (tr != _incoming_transfers.end())
                        ++tr;
                }
            }
            /* check for stale outgoing transfers, it may happen that the ACK didn't arrive, 
            it is not ACKed back, so that can happen */
            tr = _outgoing_transfers.begin();
            while (tr != _outgoing_transfers.end())
            {
                if (older_than(tr->timestamp_newest(), _drop_time))
                {
                    /* drop the outgoing transfer because it is inactive for too long */
                    tr = _outgoing_transfers.erase(tr);
                }
                else
                    ++tr;
            }
        }

        void transmit(transfer t)
        {
            /* transmit all packets within this transfer and store it in case we get a retransmit request */
            for (index_type fragment = 0; fragment < t.fragments(); ++fragment)
                transmit_event.emit(std::move(serialize_packet(types::PACKET, fragment, t)));

            _outgoing_transfers.push_back(std::move(t));
        }


        /* fires when the handler wants to transmit a packet */
        subject<interface::packet> transmit_event;
        /* fires when the handler receives and fully reconstructs a packet */
        subject<transfer> transfer_receive_event;
        subject<transfer> transfer_ack_event;

        private:

        header make_header(types type, index_type fragment, const transfer & t)
        {
            return header(type, fragment, t.size(), t.get_id(), t.get_prev_id());
        }

        /* copy the data of the fragment within the transfer and create an interface::packet from it */
        interface::packet serialize_packet(types type, index_type fragment, const transfer & t)
        {
            bytes b = to_bytes(
                make_header(type, fragment, t),
                t.at(fragment).data().size()
            );
            b.push_back(t.at(fragment).data());
            return interface::packet(t.destination(), std::move(b));
        }

        void handle_packet(const header & h, interface::packet && p)
        {
            if (h.type() == types::PACKET)
            {
                /* branch for handling _incoming_transfers */
                /* check if we already know that incoming transfer ID */
                auto tr_it = std::find_if(_incoming_transfers.begin(), _incoming_transfers.end(), 
                    [&](const auto & t){return t.get_id() == h.get_id() && t.match(p);});

                /* handle the reception of user packets and their ordering */
                if (tr_it == _incoming_transfers.end())
                {
                    /* we don't know this transfer ID, add it */
                    auto t = _incoming_transfers.emplace_back(transfer(h, this));
                    t.assign(h.fragment(), std::move(p));
                }
                else
                {
                    /* the ID is in known transfers, we need to add the incoming interface::packet to it */
                    tr_it->assign(h.fragment(), std::move(p));
                }

            }
            else
            {
                /* branch for handling _outgoing_transfers */
                auto tr_it = std::find_if(_outgoing_transfers.begin(), _outgoing_transfers.end(), 
                    [&](const auto & t){return t.get_id() == h.get_id() && t.match_as_response(p);});
                
                if (tr_it != _incoming_transfers.end())
                {
                    if (h.type() == types::PACKET_REQ)
                    {
                        /* fullfill the retransmit request */
                        transmit_event.emit(std::move(serialize_packet(types::PACKET, 
                            std::distance(_outgoing_transfers.begin(), tr_it), *tr_it)));
                    }
                    else if (h.type() == types::PACKET_ACK)
                    {
                        /* emit the ACK even for the sender and destroy this outgoing transfer
                        since we've done our job and don't need it anymore */
                        transfer_ack_event.emit(std::move(*tr_it));
                        tr_it = _outgoing_transfers.erase(tr_it);
                    }
                }
            }
        }


        std::list<transfer> _incoming_transfers;
        std::list<transfer> _outgoing_transfers;
        clock::duration _retransmit_time, _drop_time;
    };

}








#endif
