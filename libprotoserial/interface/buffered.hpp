
#include "libprotoserial/interface/interface.hpp"

namespace sp
{
    class buffered_interface : public interface
    {
        buffered_interface(std::string name, address_type address, uint max_queue_size, uint buffer_size):
            interface(name, address, max_queue_size), _rx_buffer(buffer_size) {}

        protected:

        /* iterator pointing into the _rx_buffer, it supports wrapping */
        struct buffer_iterator 
        {
            buffer_iterator(bytes::iterator begin, bytes::iterator end, bytes::iterator start) : 
                _begin(begin), _end(end), _current(start) {}

            buffer_iterator(const bytes & buff, bytes::iterator start) : 
                buffer_iterator(buff.begin(), buff.end(), start) {}

            byte & operator*() const { return *_current; }
            byte * operator->() { return _current; }

            // Prefix increment
            buffer_iterator& operator++() 
            {
                _current++; 
                if (_current >= _end) _current = _begin;
                return *this;
            }  

            buffer_iterator& operator--() 
            {
                _current--; 
                if (_current < _begin) _current = _end - 1;
                return *this;
            }  

            // Postfix increment
            //iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const buffer_iterator& a, const buffer_iterator& b) { return a._current == b._current; };
            friend bool operator!= (const buffer_iterator& a, const buffer_iterator& b) { return a._current != b._current; };

            private:
            /* _begin is the fisrt byte of the container, _end is one past the last byte of the container, 
            _current is in the interval [_begin, _end) */
            bytes::iterator _begin, _end, _current;
        }; 

        /* returns the interator that points to the beggining of the _rx_buffer, store this in
        member variable at init and use it to access the buffer */
        buffer_iterator get_rx_buffer() {return buffer_iterator(_rx_buffer, _rx_buffer.begin());}

        private:

        bytes _rx_buffer;       
    };
} // namespace sp


