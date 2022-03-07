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
#include "libprotoserial/interface/headers.hpp"
#include "libprotoserial/interface/parsers.hpp"
#include "libprotoserial/clock.hpp"

#include <iostream>

//#define SP_FRAGMENTATION_DEBUG
#define SP_FRAGMENTATION_WARNING


#ifdef SP_FRAGMENTATION_DEBUG
define(SP_FRAGMENTATION_WARNING)
#endif

namespace sp
{
    class fragmentation_handler
    {
        public:

        using header = headers::fragment_header_8b16b;
        using types = header::message_types;
        using index_type = typename header::index_type;
        using size_type = interface::packet::data_type::size_type;
        
        /* we'd like to represent something like 115200bps, we could use clock::rep's per bit but
        that would restrict us when the bit rate is higher than the ticks -> use fraction? */
        //using bitrate_type = 


        /* there should never be a need for the user to construct this, it is provided by the
         * fragmentation_handler as fully initialized object ready to be used, user should never
         * need to use function that start with underscore 
         * 
         * there are two modes of usage from within the fragmentation_handler:
         * 1)  transfer is constructed using the transfer(const Header & h, fragmentation_handler * handler)
         *     constructor when fragmentation_handler encounters a new ID and such transfer is put into the
         *     _incoming_transfers list, where it is gradually filled with incoming packets from the interface.
         *     This is a mode where transfer's internal vector gets preallocated to a known number, so functions
         *     _is_complete(), _missing_fragment() and _missing_fragments_total() make sense to call.
         * 
         * 2)  transfer is created using the new_transfer() function, where it is constructed by the
         *     transfer(fragmentation_handler * handler, id_type id, id_type prev_id = 0) constructor on user
         *     demand. This transfer will, presumably, be transmitted some time in the future. The internal
         *     vector is empty and gets dynamically larger as user uses the push_front(bytes) and push_back(bytes)
         *     functions. 
         *     In contrast to the first scenario data will no longer satisfy the data_size() <= max_packet_size()
         *     property, which is required for transmit, so function the _get_fragment() is used by the 
         *     fragmentation_handler to create packets that do.
         * 
         */
        class transfer
        {
            public:
            
            /* as with interface::address_type this is a type that can hold all used 
            fragmentation_handler::id_type types */
            using id_type = uint;
            using index_type = uint;

            /* constructor used when the fragmentation_handler receives the first piece of the 
            packet - when new a packet transfer is initiated by other peer. This initial packet
            does not need to be the first fragment, fragmentation_handler is responsible for 
            the correct order of interface::packets within this object */
            template<class Header>
            transfer(const Header & h, fragmentation_handler * handler) :
                _data(h.fragments_total()), _handler(handler), _id(h.get_id()), _prev_id(h.get_prev_id()) {}

            /* constructor used by the fragmentation_handler in new_transfer */
            transfer(fragmentation_handler * handler, id_type id, id_type prev_id = 0):
                _handler(handler), _id(id), _prev_id(prev_id) {}

            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _begin() {return _data.begin();}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _begin() const {return _data.begin();}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _end() {return _data.end();}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _end() const {return _data.end();}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _last() {return _data.empty() ? _data.begin() : std::prev(_data.end());}
            auto _last() const {return _data.empty() ? _data.begin() : std::prev(_data.end());}

            void _push_back(const interface::packet & p) {refresh(p); _data.push_back(p.data());}
            void _push_back(interface::packet && p) {refresh(p); _data.push_back(std::move(p.data()));}
            void _assign(index_type fragment, const interface::packet & p) {refresh(p); _data.at(fragment - 1) = p.data();}
            void _assign(index_type fragment, interface::packet && p) {refresh(p); _data.at(fragment - 1) = std::move(p.data());}

            /* expose the internal data, used in fragmentation_handler and data_iterator */
            //auto _at(size_t pos) {return _data.at(pos);}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            //auto _at(size_t pos) const {return _data.at(pos);}
            //auto _size() const {return _data.size();}

            struct data_iterator
            {
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = interface::packet::data_type::difference_type;
                using value_type        = interface::packet::data_type::value_type;
                using pointer           = interface::packet::data_type::const_pointer; 
                using reference         = interface::packet::data_type::const_reference;

                data_iterator(const transfer * p, bool is_begin) : 
                    _packet(p) 
                {
                    if (is_begin)
                    {
                        /* initialize the iterators to the first byte of the first interface::packet */
                        _ipacket = _packet->_begin();
                        if (_ipacket != _packet->_end())
                        {
                            _ipacket_data = _ipacket->begin();
                            while(_ipacket_data == _ipacket->end() && _ipacket != _packet->_last())
                            {
                                ++_ipacket;
                                _ipacket_data = _ipacket->begin();
                            }
                        }
                        else
                            _ipacket_data = nullptr;
                    }
                    else
                    {
                        /* initialize the iterators to the end of data of the last interface::packet */
                        _ipacket = _packet->_last();
                        if (_ipacket != _packet->_end())
                            _ipacket_data = _ipacket->end();
                        else
                            _ipacket_data = nullptr;
                    }
                }

                reference operator*() const { return *_ipacket_data; }
                pointer operator->() const { return _ipacket_data; }

                // Prefix increment
                data_iterator& operator++() 
                {
                    /* we want to increment by one, try to increment the data iterator
                    within the current interface::packet */
                    ++_ipacket_data;
                    /* when we are at the end of data of current interface::packet, we need
                    to advance to the next interface::packet and start from the beginning of
                    its data */
                    if (_ipacket_data == _ipacket->end() && _ipacket != _packet->_last())
                    {
                        do {
                            ++_ipacket;
                            _ipacket_data = _ipacket->begin();
                        } while (_ipacket_data == _ipacket->end() && _ipacket != _packet->_last());
                    }
                    return *this;
                }

                data_iterator& operator+=(uint shift)
                {
                    //FIXME
                    while (shift > 0)
                    {
                        this->operator++();
                        --shift;
                    }
                    return *this; 
                }

                friend data_iterator operator+(data_iterator lhs, uint rhs)
                {
                    lhs += rhs;
                    return lhs; 
                }

                friend bool operator== (const data_iterator& a, const data_iterator& b) 
                    { return a._ipacket_data == b._ipacket_data && a._ipacket == b._ipacket; };
                friend bool operator!= (const data_iterator& a, const data_iterator& b) 
                    { return a._ipacket_data != b._ipacket_data || a._ipacket != b._ipacket; };

                private:
                //TODO rename
                const transfer * _packet;
                std::vector<interface::packet::data_type>::const_iterator _ipacket;
                interface::packet::data_type::const_iterator _ipacket_data;
            };

            /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
            auto data_begin() const {return data_iterator(this, true);}
            /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
            auto data_end() const {return data_iterator(this, false);}

            bytes::size_type data_size() const 
            {
                bytes::size_type ret = 0;
                for (auto it = _begin(); it != _end(); ++it) ret += it->size();
                return ret;
            }

            /* copies all the internally stored data into one bytes container and returns it */
            bytes data_contiguous() const
            {
                bytes b(0, 0, data_size());
                for (auto it = _begin(); it != _end(); ++it) b.push_back(*it);
                return b;
            }
            
            void push_back(const bytes & b) {refresh(); _data.push_back(b);}
            void push_back(bytes && b) {refresh(); _data.push_back(std::move(b));}
            void push_front(const bytes & b) {refresh(); _data.insert(_data.begin(), b);}
            void push_front(bytes && b) {refresh(); _data.insert(_data.begin(), std::move(b));}

            /* the packet id is used to uniquely identify a packet transfer together with the destination and source
            addresses and the interface name. It is issued by the transmittee of the packet */
            id_type get_id() const {return _id;}
            id_type get_prev_id() const {return _prev_id;}
            interface::address_type destination() const {return _destination;}
            void set_destination(interface::address_type dst) {_destination = dst;}
            interface::address_type source() const {return _source;}
            const sp::interface * get_interface() const {return _interface;}
            const fragmentation_handler * get_handler() const {return _handler;}
            clock::time_point timestamp_creation() const {return _timestamp_creation;}
            clock::time_point timestamp_modified() const {return _timestamp_modified;}

            /* checks if p's addresses and interface match the transfer's, this along with id match means that p 
            should be part of this transfer */
            bool match(const interface::packet & p) const 
                {return p.destination() == destination() && p.source() == source();}

            /* checks p's addresses as a response to this transfer and interface match the transfer's */
            bool match_as_response(const interface::packet & p) const 
                {return p.source() == destination();}
            

            /* returns the number of packets needed to transmit this transfer,
            this obviously depends on the mode but we can make some assumptions:
            -   when in mode1 transfer has internally preallocated the needed number of 
                slots for fragments so this returns that number
            -   when in mode 2 slots are allocated on demand with new data, so this 
                returns the computed number of needed fragments based on the data size */
            index_type _fragments_count() const 
            {
                if (_is_complete())
                    /* mode 2 */
                    return data_size() / _handler->max_packet_size() + 
                        (data_size() % _handler->max_packet_size() == 0 ? 0 : 1);
                else
                    /* mode 1 */
                    return _data.size();
            }
            
            /* mode 1) only */
            bool _is_complete() const
            {
                for (auto b = _begin(); b != _end(); ++b)
                {
                    if (b->is_empty())
                        return false;
                }
                return true;
            }
            /* mode 1) only */
            index_type _missing_fragment() const
            {
                for (std::size_t i = 0; i < _data.size(); ++i)
                {
                    if (_data.at(i).is_empty())
                        return (index_type)(i + 1);
                }
                return 0; //_fragments_count();
            }
            /* mode 2) only, preferably */
            interface::packet _get_fragment(index_type fragment) const
            {
                if (fragment == 0) throw std::invalid_argument("fragment == 0");
                fragment -= 1;
                auto p_size = _handler->max_packet_size();
                if (fragment * p_size > data_size()) throw std::invalid_argument("fragment * p_size > data_size()");

                bytes data(sizeof(header), 0, p_size);
                auto b = data_begin() + fragment * p_size, e = data_end();
                for (; b != e && data.size() < p_size; ++b) data.push_back(*b);
                
                return interface::packet(destination(), std::move(data));
            }

            friend std::ostream& operator<<(std::ostream& os, const sp::fragmentation_handler::transfer & t) 
            {
                os << "dst: " << t.destination() << ", src: " << t.source();
                os << ", int: " << (t.get_interface() ? t.get_interface()->name() : "null");
                os << ", id: " << t.get_id() << ", prev_id: " << t.get_prev_id();
                //auto f = t._fragments_count();
                //os << ", (" << (f - t._missing_fragments_total()) << '/' << f << ")";
                os << ", " << t.data_contiguous();
                return os;
            }

            friend std::ostream& operator<<(std::ostream& os, const sp::fragmentation_handler::transfer * t) 
            {
                if (t) os << *t; else os << "null transfer";
                return os;
            }

            private:

            void refresh() {_timestamp_modified = clock::now();}
            void refresh(const interface::packet & p)
            {
                refresh();
                _source = p.source();
                _destination = p.destination();
                _interface = p.get_interface();
            }

            std::vector<interface::packet::data_type> _data;
            clock::time_point _timestamp_creation, _timestamp_modified;
            interface::address_type _source = 0, _destination = 0;
            const interface * _interface = nullptr;
            fragmentation_handler * _handler = nullptr;
            id_type _id = 0, _prev_id = 0;
        };

        private:
        
        /* this wraps the underlaying transfer to strip it off otherwise unneeded values
        like timeouts and various housekeeping stuff */
        struct transfer_progress
        {
            /* using a pointer here to free up space when we get rid of the transfer once it's done 
            but need to keep the transfer_progress object for a while longer */
            std::unique_ptr<transfer> tr;
            clock::time_point timestamp_accessed;
            uint retransmitions = 0;
            /* this should always match the tr->get_id() */
            transfer::id_type id;

            transfer_progress(transfer && t) :
                tr(std::unique_ptr<transfer>(new transfer(std::move(t)))), timestamp_accessed(clock::now()), id(tr->get_id()) {}

            void transmit_done() {timestamp_accessed = clock::now();}
            void retransmit_done() {timestamp_accessed = clock::now(); ++retransmitions;}
        };


        public:

        
        fragmentation_handler(size_type max_packet_size, clock::duration retransmit_time, clock::duration drop_time, 
            uint retransmit_multiplier) :
                _retransmit_time(retransmit_time), _drop_time(drop_time), _max_packet_size(max_packet_size - sizeof(header)),
                _retransmit_multiplier(retransmit_multiplier) {}


        /* the callback handles the incoming packets, it does not handle any timeouts, sending requests, 
        or anything that assumes periodicity, the main_task is for that */
        void receive_callback(interface::packet p) noexcept
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "receive_callback got: " << p << std::endl;
#endif
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
            auto it = _incoming_transfers.begin();
            while (it != _incoming_transfers.end())
            {
                if (!it->tr)
                {
                    /* we have the transfer_progress object but it does not own any transfer, which means
                    that the block below must have been executed in the near past
                    drop the transfer_progress after some sufficiently long period since its last access */
                    //TODO get rid of the 5 and make it a config thing somehow
                    if (older_than(it->timestamp_accessed, _drop_time * 5))
                        it = _incoming_transfers.erase(it);
                }
                else if (it->tr->_is_complete())
                {
                    /* checks whether any of the transfers is complete, if so emit it as receive event */
                    transmit_event.emit(std::move(
                        interface::packet(it->tr->source(), 
                        to_bytes(make_header(types::PACKET_ACK, it->tr->_fragments_count(), *it->tr)))
                    ));
                    transfer_receive_event.emit(std::move(*it->tr));
                    it->tr.reset();
                    /* std::move moved the local copy of the transfer out of the handler, but we
                    will hold onto that transfer_progress structure for a while longer. in case
                    the ACK packet gets lost, the source will try to retransmit this transfer
                    thinking that we didn't notice it */
                }
                else
                {
                    if (older_than(it->tr->timestamp_modified(), _drop_time))
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "timed out incoming: " << it->tr << std::endl;
#endif
                        /* drop the incoming transfer because it is inactive for too long */
                        it = _incoming_transfers.erase(it);
                    }
                    else if (older_than(it->tr->timestamp_modified(), _retransmit_time) && 
                        older_than(it->timestamp_accessed, _retransmit_time))
                    {
                        /* find the missing packet's index and request a retransmit */
                        auto index = it->tr->_missing_fragment();
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "requesting retransmit for id " << it->id << " index " << index << std::endl;
#endif
                        transmit_event.emit(std::move(
                            interface::packet(it->tr->source(), 
                            to_bytes(make_header(types::PACKET_REQ, index, *it->tr)))
                        ));
                        it->retransmit_done();
                    }
                }
                /* check again because an erase can happen in this branch as well, 
                we don't need to worry about skipping a transfer when the erase happens
                since we will return into this function later anyway */
                if (it != _incoming_transfers.end())
                    ++it;
            }
            /* check for stale outgoing transfers, it may happen that the ACK didn't arrive, 
            it is not ACKed back, so that can happen */
            it = _outgoing_transfers.begin();
            while (it != _outgoing_transfers.end())
            {
                if (older_than(it->timestamp_accessed, _drop_time))
                {
                    /* drop the outgoing transfer because it is inactive for too long */
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "timed out outgoing id " << it->id << std::endl;
#endif
                    it = _outgoing_transfers.erase(it);
                }
                else if (it->retransmitions < it->tr->_fragments_count() * _retransmit_multiplier &&
                    older_than(it->timestamp_accessed, _retransmit_time))
                {
                    /* either the destination is dead or the first fragment got lost during
                    transit, try to retransmit the first fragment */
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "retransmitting first fragment of id " << it->id << std::endl;
#endif
                    transmit_event.emit(std::move(serialize_packet(types::PACKET, 1, *it->tr)));
                    it->retransmit_done();
                }
                else
                    ++it;
            }
        }

        void transmit(transfer t)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "transmit got: " << t << std::endl;
#elif defined(SP_FRAGMENTATION_WARNING)
            std::cout << "transmit got id " << t.get_id() << std::endl;
#endif
            /* transmit all packets within this transfer and store it in case we get a retransmit request */
            for (index_type fragment = 1; fragment <= t._fragments_count(); ++fragment)
            {
#ifdef SP_FRAGMENTATION_DEBUG
                std::cout << "transmit emitting event" << std::endl;
#endif
                transmit_event.emit(std::move(serialize_packet(types::PACKET, fragment, t)));
            }

            //auto tp = transfer_progress(std::move(t));
            auto & tp = _outgoing_transfers.emplace_back(std::move(t));
            tp.transmit_done();
        }

        transfer new_transfer()
        {
            return transfer(this, new_id(), 0);
        }

        size_type max_packet_size() const
        {
            return _max_packet_size;
        }

        void print_debug() const
        {
            std::cout << "incoming_transfers: " << _incoming_transfers.size() << std::endl;
            for (const auto & t : _incoming_transfers)
                std::cout << t.tr << std::endl;
            
            std::cout << "outgoing_transfers: " << _outgoing_transfers.size() << std::endl;
            for (const auto & t : _outgoing_transfers)
                std::cout << t.tr << std::endl;
        }

        //const interface * get_interface() const {return _interface;}

        /* fires when the handler wants to transmit a packet */
        subject<interface::packet> transmit_event;
        /* fires when the handler receives and fully reconstructs a packet */
        subject<transfer> transfer_receive_event;
        subject<transfer> transfer_ack_event;

        private:

        transfer::id_type new_id()// 89[0] 180[0] 389[0] 
        {
            if (++_id_counter == 0) ++_id_counter;
            return _id_counter;
        }

        header make_header(types type, index_type fragment, const transfer & t)
        {
            return header(type, fragment, t._fragments_count(), t.get_id(), t.get_prev_id());
        }

        /* copy the data of the fragment within the transfer and create an interface::packet from it */
        interface::packet serialize_packet(types type, index_type fragment, const transfer & t)
        {
            auto p = t._get_fragment(fragment);
            bytes h = to_bytes(make_header(type, fragment, t));
            p.data().push_front(std::move(h));
            return p;
        }

        void handle_packet(const header & h, interface::packet && p)
        {
#ifdef SP_FRAGMENTATION_DEBUG
            std::cout << "handle_packet got: " << p << std::endl;
#endif
            /* handle the reception of user packets and their ordering */
            if (h.type() == types::PACKET)
            {
                /* branch for handling _incoming_transfers */
                /* check if we already know that incoming transfer ID */
                auto it = std::find_if(_incoming_transfers.begin(), _incoming_transfers.end(), 
                    [&](const auto & t){
                        //FIXME use tr->match() in both cases, as per spec
                        if (t.tr)
                            return t.tr->get_id() == h.get_id() && t.tr->match(p);
                        else
                            return t.id == h.get_id();
                });

                if (it == _incoming_transfers.end())
                {
#ifdef SP_FRAGMENTATION_DEBUG
                    std::cout << "creating new incoming transfer id " << h.get_id() << std::endl;
#endif
                    /* we don't know this transfer ID, create new incoming transfer */
                    auto& t = _incoming_transfers.emplace_back(transfer_progress(transfer(h, this)));
                    t.tr->_assign(h.fragment(), std::move(p));
                }
                else
                {
                    /* we know this ID, now we need to check if we have already received this transfer and
                    is therefor duplicate, or whether we are in the process of receiving it */
                    if (it->tr)
                    {
#ifdef SP_FRAGMENTATION_DEBUG
                        std::cout << "assigning to existing incoming transfer id " << h.get_id() << " at " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        /* the ID is in known transfers, we need to add the incoming interface::packet to it */
                        it->tr->_assign(h.fragment(), std::move(p));
                    }
                    else
                    {
                        /* we, for some reason, received a fragment of already received transfer, the ACK
                        from us probably got lost in transit, just reply with another ACK and ignore this fragment */
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "sending ACK for already received id " << h.get_id() << std::endl;
#endif
                        transmit_event.emit(std::move(
                            interface::packet(p.source(), 
                            std::move(to_bytes(header(types::PACKET_ACK, h.fragment(), h.fragments_total(), h.get_id(), h.get_prev_id()))))
                        ));
                    }
                }
            }
            else
            {
                /* branch for handling _outgoing_transfers */
                auto it = std::find_if(_outgoing_transfers.begin(), _outgoing_transfers.end(), 
                    [&](const auto & t){return t.tr->get_id() == h.get_id() && t.tr->match_as_response(p);});
                
                if (it != _outgoing_transfers.end())
                {
                    if (h.type() == types::PACKET_REQ)
                    {
#ifdef SP_FRAGMENTATION_WARNING
                        std::cout << "handling retransmit request of id " << h.get_id() << " fragment " << (int)h.fragment() << " of " << (int)h.fragments_total() << std::endl;
#endif
                        /* fullfill the retransmit request */
                        transmit_event.emit(std::move(serialize_packet(types::PACKET, h.fragment(), *it->tr)));
                        it->retransmit_done();
                    }
                    else if (h.type() == types::PACKET_ACK)
                    {
#ifdef SP_FRAGMENTATION_DEBUG
                        std::cout << "got packet ACK for id " << h.get_id() << std::endl;
#endif
                        /* emit the ACK event for the sender and destroy this outgoing transfer
                        since we've done our job and don't need it anymore in contrast to the 
                        incoming transfer where the transmitted ACK may not be received, here we
                        are be sure */
                        transfer_ack_event.emit(std::move(*it->tr));
                        it = _outgoing_transfers.erase(it);
                    }
                }
            }
        }


        std::list<transfer_progress> _incoming_transfers, _outgoing_transfers;
        clock::duration _retransmit_time, _drop_time;
        transfer::id_type _id_counter = 0;
        size_type _max_packet_size;
        uint _retransmit_multiplier;
    };

}






#endif
