/* 
 * The point of the packet is to provide a useful abstraction for the 
 * fragmentation_handler. It holds multiple interface::packets and provides
 * an iterator to access that data without copying.
 * 
 * For this it needs to
 * - store packets and keep their order, allow inserting packets to requested
 *   positions (like a linked list)
 * - 
 * 
 * 
 */




#ifndef _SP_TRANSFER
#define _SP_TRANSFER

#include "libprotoserial/interface.hpp"

#include <vector>
#include <algorithm>


namespace sp
{
    class fragmentation_handler;
    
    class transfer
    {
        public:

        using packets_container = std::vector<interface::packet>;
        
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
            _packets(h.fragments_total()), _handler(handler), _id(h.get_id()), _prev_id(h.get_prev_id()) {}

        auto begin() {return _packets.begin();}
        auto begin() const {return _packets.begin();}
        auto cbegin() const {return _packets.cbegin();}
        auto end() {return _packets.end();}
        auto end() const {return _packets.end();}
        auto cend() const {return _packets.cend();}

        auto at(size_t pos) {return _packets.at(pos);}
        auto at(size_t pos) const {return _packets.at(pos);}

        auto last() {return _packets.empty() ? _packets.begin() : std::prev(_packets.end());}
        auto size() const {return _packets.size();}
        index_type fragments() const {return _packets.size();}


        /* presents the data of internally stored list of interface::packets as contiguous */
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
                    _ipacket = _packet->begin();
                    if (_ipacket != _packet->end())
                        _ipacket_data = _ipacket->data().begin();
                    else
                        _ipacket_data = nullptr;
                }
                else
                {
                    /* initialize the iterators to the end of data of the last interface::packet */
                    _ipacket = _packet->last();
                    if (_ipacket != _packet->end())
                        _ipacket_data = _ipacket->data().end();
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
                _ipacket_data++;
                /* when we are at the end of data of current interface::packet, we need
                to advance to the next interface::packet and start from the beginning of
                its data */
                if (_ipacket_data == _ipacket->data().end() && _ipacket != _packet->last())
                {
                    _ipacket++;
                    _ipacket_data = _ipacket->data().begin();
                }
                return *this;
            }

            friend bool operator== (const data_iterator& a, const data_iterator& b) 
                { return a._ipacket_data == b._ipacket_data && a._ipacket == b._ipacket; };
            friend bool operator!= (const data_iterator& a, const data_iterator& b) 
                { return a._ipacket_data != b._ipacket_data || a._ipacket != b._ipacket; };

            private:
            transfer * _packet;
            packets_container::iterator _ipacket;
            interface::packet::data_type::iterator _ipacket_data;
        };

        /* data_iterator presents the data of internally stored list of interface::packets as contiguous */
        auto data_begin() {return data_iterator(this, true);}
        /* data_iterator presents the data of internally stored list of interface::packets as contiguous */
        auto data_end() {return data_iterator(this, false);}
        
        /* takes the iterator from packets_begin and packets_end functions, behaves just like std::list::insert */
        //auto insert(packets_container::const_iterator pos, const interface::packet & p) {_packets.insert(pos, p);}
        /* takes the iterator from packets_begin and packets_end functions, behaves just like std::list::insert */
        //auto insert(packets_container::const_iterator pos, interface::packet && p) {_packets.insert(pos, p);}
        
        /* takes the iterator from packets_begin and packets_end functions, behaves just like std::list::push_back */
        void push_back(const interface::packet & p) {_packets.push_back(p);}
        /* takes the iterator from packets_begin and packets_end functions, behaves just like std::list::push_back */
        void push_back(interface::packet && p) {_packets.push_back(p);}

        void assign(index_type fragment, const interface::packet & p) {_packets.at(fragment) = p;}
        void assign(index_type fragment, interface::packet && p) {_packets.at(fragment) = p;}

        /* the packet id is used to uniquely identify a packet transfer together with the destination and source
        addresses and the interface name. It is issued by the transmittee of the packet */
        id_type get_id() const {return _id;}
        id_type get_prev_id() const {return _prev_id;}
        /* packet object does not ensure that the destination address is consistent thrughout the child interface::packet objects */
        interface::address_type destination() const {return _packets.empty() ? 0 : _packets.front().destination();}
        /* packet object does not ensure that the source address is consistent thrughout the child interface::packet objects */
        interface::address_type source() const {return _packets.empty() ? 0 : _packets.front().source();}
        /* packet object does not ensure that the interface is consistent thrughout the child interface::packet objects */
        const sp::interface * get_interface() const {return _packets.empty() ? nullptr : _packets.front().get_interface();}
        //FIXME what to return when _packets.empty()?
        clock::time_point timestamp_oldest() const {return std::min_element(_packets.begin(), _packets.end(), 
            [](const auto & a, const auto & b) {return a.timestamp() < b.timestamp();})->timestamp();}
        clock::time_point timestamp_newest() const {return std::max_element(_packets.begin(), _packets.end(), 
            [](const auto & a, const auto & b) {return a.timestamp() < b.timestamp();})->timestamp();}

        bool is_complete() const
        {
            for (auto p = begin(); p != end(); ++p)
            {
                if (!p->carries_information())
                    return false;
            }
            return true;
        }

        index_type missing_fragment() const
        {
            for (auto p = begin(); p != end(); ++p)
            {
                if (!p->carries_information())
                    return std::distance(begin(), p);
            }
            return fragments();
        }

        /* checks if p's addresses and interface match the transfer's, this along with id match means that p 
        should be part of this transfer */
        bool match(const interface::packet & p) const 
            {return p.destination() == destination() && p.source() == source() && p.get_interface() == get_interface();}

        /* checks p's addresses as a response to this transfer and interface match the transfer's */
        bool match_as_response(const interface::packet & p) const 
            {return p.source() == destination() && p.get_interface() == get_interface();}

        private:

        packets_container _packets;
        fragmentation_handler * _handler;
        id_type _id, _prev_id;
    };
}





#endif

