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
        using size_type = interface::packet::data_type::size_type;
        
        /* we'd like to represent something like 115200bps, we could use clock::rep's per bit but
        that would restrict us when the bit rate is higher than the ticks -> use fraction? */
        //using bitrate_type = 


        /* there should never be a need for the user to construct this, it is provided by the
        fragmentation_handler as fully initialized object ready to be used, user should never
        need to use function that start with underscore */
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

            /* expose the internal data, used in fragmentation_handler and data_iterator */
            //auto _at(size_t pos) {return _data.at(pos);}
            /* expose the internal data, used in fragmentation_handler and data_iterator */
            auto _at(size_t pos) const {return _data.at(pos);}
            auto _size() const {return _data.size();}

            struct data_iterator
            {
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = interface::packet::data_type::difference_type;
                using value_type        = interface::packet::data_type::value_type;
                using pointer           = interface::packet::data_type::pointer; 
                using reference         = interface::packet::data_type::reference;

                data_iterator(transfer * p, bool is_begin) : 
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
                pointer operator->() { return _ipacket_data; }

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
                transfer * _packet;
                std::vector<interface::packet::data_type>::iterator _ipacket;
                interface::packet::data_type::iterator _ipacket_data;
            };

            /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
            auto data_begin() {return data_iterator(this, true);}
            /* data_iterator exposes the potentially fragmented internally stored data as contiguous */
            auto data_end() {return data_iterator(this, false);}

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

            void _push_back(const interface::packet & p) {refresh(p); _data.push_back(p.data());}
            void _push_back(interface::packet && p) {refresh(p); _data.push_back(std::move(p.data()));}
            void _assign(index_type fragment, const interface::packet & p) {refresh(p); _data.at(fragment) = p.data();}
            void _assign(index_type fragment, interface::packet && p) {refresh(p); _data.at(fragment) = std::move(p.data());}
            
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

            index_type _fragments_count() const {return _data.size();}

            bool _is_complete() const
            {
                for (auto b = _begin(); b != _end(); ++b)
                {
                    if (b->is_empty())
                        return false;
                }
                return true;
            }

            index_type _missing_fragment() const
            {
                for (auto b = _begin(); b != _end(); ++b)
                {
                    if (b->is_empty())
                        return std::distance(_begin(), b);
                }
                return _fragments_count();
            }

            index_type _missing_fragments_total() const
            {
                index_type ret = 0;
                for (auto b = _begin(); b != _end(); ++b)
                {
                    if (b->is_empty())
                        ++ret;
                }
                return ret;
            }

            interface::packet _get_fragment(size_type size, index_type fragment)
            {
                bytes data(0, 0, size);
                auto b = data_begin() + fragment * size, e = data_end();
                for (; b != e; ++b) data.push_back(*b);
                return interface::packet(destination(), std::move(data));
            }

            /* checks if p's addresses and interface match the transfer's, this along with id match means that p 
            should be part of this transfer */
            bool match(const interface::packet & p) const 
                {return p.destination() == destination() && p.source() == source() && p.get_interface() == get_interface();}

            /* checks p's addresses as a response to this transfer and interface match the transfer's */
            bool match_as_response(const interface::packet & p) const 
                {return p.source() == destination() && p.get_interface() == get_interface();}

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

        
        fragmentation_handler(size_type max_packet_size, clock::duration retransmit_time, clock::duration drop_time) :
            _retransmit_time(retransmit_time), _drop_time(drop_time), _max_packet_size(max_packet_size) {}


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
                if (tr->_is_complete())
                {
                    /* checks whether any of the transfers is complete, if so emits 
                    the receive_event and removes that transfer from _incoming_transfers */
                    transfer_receive_event.emit(std::move(*tr));
                    tr = _incoming_transfers.erase(tr);
                }
                else
                {
                    //TODO timeout, retransmit request atd
                    if (older_than(tr->timestamp_modified(), _drop_time))
                    {
                        /* drop the incoming transfer because it is inactive for too long */
                        tr = _incoming_transfers.erase(tr);
                    }
                    else if (older_than(tr->timestamp_modified(), _retransmit_time))
                    {
                        /* find the missing packet's index and request a retransmit */
                        auto index = tr->_missing_fragments_total();
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
                if (older_than(tr->timestamp_modified(), _drop_time))
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
            for (index_type fragment = 0; fragment < t._fragments_count(); ++fragment)
                transmit_event.emit(std::move(serialize_packet(types::PACKET, fragment, t)));

            _outgoing_transfers.push_back(std::move(t));
        }

        transfer new_transfer()
        {
            return transfer(this, new_id(), 0);
        }

        //const interface * get_interface() const {return _interface;}

        /* fires when the handler wants to transmit a packet */
        subject<interface::packet> transmit_event;
        /* fires when the handler receives and fully reconstructs a packet */
        subject<transfer> transfer_receive_event;
        subject<transfer> transfer_ack_event;

        private:

        transfer::id_type new_id()
        {
            if (++_id_counter == 0) ++_id_counter;
            return _id_counter;
        }

        header make_header(types type, index_type fragment, const transfer & t)
        {
            return header(type, fragment, t._size(), t.get_id(), t.get_prev_id());
        }

        /* copy the data of the fragment within the transfer and create an interface::packet from it */
        interface::packet serialize_packet(types type, index_type fragment, const transfer & t)
        {
            bytes b = to_bytes(
                make_header(type, fragment, t),
                t._at(fragment).size()
            );
            b.push_back(t._at(fragment));
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
                    /* we don't know this transfer ID, create new incoming transfer */
                    auto t = _incoming_transfers.emplace_back(transfer(h, this));
                    t._assign(h.fragment(), std::move(p));
                }
                else
                {
                    /* the ID is in known transfers, we need to add the incoming interface::packet to it */
                    tr_it->_assign(h.fragment(), std::move(p));
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
        transfer::id_type _id_counter = 0;
        size_type _max_packet_size;
    };

}


std::ostream& operator<<(std::ostream& os, const sp::fragmentation_handler::transfer & t) 
{
    os << "dst: " << t.destination() << ", src: " << t.source();
    os << ", int: " << (t.get_interface() ? t.get_interface()->name() : "null");
    os << ", (" << t._missing_fragments_total() << '/' << t._fragments_count() << ")";
    os << ", " << t.data_contiguous();
    return os;
}





#endif
