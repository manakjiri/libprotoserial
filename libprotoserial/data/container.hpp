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

/*
 * protocols need a byte container, which not only can reserve space in the back, but
 * also, perhaps more importantly, at the front of the data
 * 
 * 
 */

#ifndef _SP_CONTAINER
#define _SP_CONTAINER


#include <libprotoserial/data/byte.hpp>

#include <initializer_list>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <memory>

#ifndef SP_NO_IOSTREAM
#include <iostream>
#endif


namespace sp
{
    using uint = unsigned int;
    using out_of_range = std::out_of_range;

    class bytes 
    {
        public:

        typedef byte                value_type;
        typedef uint                size_type;
        typedef int                 difference_type;
        typedef value_type&         reference;
        typedef const value_type&   const_reference;
        typedef value_type*         pointer;
        typedef const value_type*   const_pointer;
        typedef pointer             iterator;
        typedef const_pointer       const_iterator;

        /* struct iterator 
        {
            // iterator tags here...

            iterator(pointer ptr) : _ptr(ptr) {}

            reference operator*() const { return *_ptr; }
            pointer operator->() { return _ptr; }

            // Prefix increment
            iterator& operator++() { _ptr++; return *this; }  

            // Postfix increment
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator== (const iterator& a, const iterator& b) { return a._ptr == b._ptr; };
            friend bool operator!= (const iterator& a, const iterator& b) { return a._ptr != b._ptr; };     

            private:

            pointer _ptr;
        }; */

        /* default */
        bytes()
        {
            _init(); 
        }
        bytes(size_type length) : bytes(0, length, 0) {}
        /* overallocation - capacity will be equal to front + length + back */
        bytes(size_type front, size_type length, size_type back) :
            bytes()
        {
            _capacity = front + length + back;
            _offset = front;
            _length = length;
            alloc(_capacity);
        }
        /* use this if you want to wrap existing raw array of bytes by this class */
        bytes(pointer data, size_type length) :
            bytes()
        {
            _data = data;
            _length = length;
        }

        bytes(std::initializer_list<value_type> values):
            bytes(values.size())
        {
            std::copy(values.begin(), values.end(), begin());
        }

        //operator std::string() const {return };
        bytes(const std::string & from) :
            bytes(from.size())
        {
            copy_from(reinterpret_cast<bytes::pointer>(const_cast<char*>(from.c_str())), from.size());
        }
        
        /* copy - only the currently exposed data gets coppied, overallocation is not used */
        bytes(const bytes & other) :
            bytes(other.size())
        {
            copy_from(other.data(), size());
        }
        bytes & operator= (const bytes & other)
        {
            _offset = 0;
            if (other.size() != _capacity) 
            {
                clear();
                alloc(other.size());
                _capacity = _length = other.size();
            }
            copy_from(other.data(), size());
            return *this;
        }
        /* move */
        bytes(bytes && other)
        {
            _data = other.get_base();
            _length = other.size();
            _offset = other.get_offset();
            _capacity = other.capacity();
            other._init();
        }
        bytes & operator= (bytes && other)
        {
            clear();
            _data = other.get_base();
            _length = other.size();
            _offset = other.get_offset();
            _capacity = other.capacity();
            other._init();
            return *this;
        }

        ~bytes()
        {
            clear();
        }
        
        
        /* returns the current data size, if overallocation is not used than size == capacity */
        size_type size() const {return _length;}
        /* a container is empty when its size is equal to 0 */
        bool is_empty() const {return size() == 0;}
        /* returns pointer to data */
        pointer data()
        {
            if (_data)
                return &_data[_offset];
            else
                return nullptr;
        }
        pointer data() const
        {
            if (_data)
                return &_data[_offset];
            else
                return nullptr;
        }
        
        const value_type & at(size_type i) const
        {
            range_check(i);
            return _data[i + _offset];
        }
        value_type & at(size_type i)
        {
            range_check(i);
            return _data[i + _offset];
        }
        const value_type & operator[] (size_type i) const {return at(i);}
        value_type & operator[] (size_type i) {return at(i);}

        iterator begin() {return data();}
        iterator end() {return data() + size();}
        //iterator begin() {return iterator(data());}
        //iterator end() {return iterator(data() + size());}
        iterator begin() const {return data();}
        iterator end() const {return data() + size();}
        const_iterator cbegin() const {return data();}
        const_iterator cend() const {return data() + size();}

        explicit operator bool() const {return !is_empty();}

        /* bool operator==(const bytes & other) const {return std::equal(cbegin(), other.cend(), cbegin(), other.cend());}
        bool operator!=(const bytes & other) const {return !((*this) == other);} */
        
        /* expands the container by the requested amount such that [front B][size B][back B], 
        front or back can be 0, in which case nothing happens */
        void expand(const size_type front, const size_type back)
        {
            reserve(front, back);
            _offset = _offset - front;
            _length = _length + front + back;
        }
        /* capacity of the container will be equal or greater than size() + front + back, size() does not change,
        this function merely reserves requested capacity by reallocation if necesary, front or back can be 0 */
        void reserve(const size_type front, const size_type back)
        {
            /* do nothing if the container has enough margin already */
            if (_offset >= front && _back() >= back)
                return;

            /* keep reference to the old buffer since we need to reallocate it */
            //std::unique_ptr<value_type> oldget_base(_data);
            pointer oldget_base = _data;
            
            /* allocate the new data buffer and update the capacity so it refelects this */
            _capacity = front + _length + back;
            alloc(_capacity);

            if (oldget_base)
            {
                /* copy the original data */
                for (size_type i = 0; i < _length; i++)
                    _data[i + front] = oldget_base[i + _offset];

                delete[] oldget_base;
            }

            /* finally update the offset because we no longer need the old value */
            _offset = front;
        }
        /* shrink the container from either side, this does not reallocate the data, just hides it
        use the shrink_to_fit function after this one to actually reduce the container size */
        void shrink(const size_type front, const size_type back)
        {
            /* do nothing */
            if (front == 0 && back == 0)
                return;
            
            /* front plus offset could run past capacity, which we don't want, 
            similarly size minus back could be negative, which would also be bad.
            if the combination of arguents yields negative size, the sane thing to do
            is to set the size to zero and ignore everything otherwise */
            if (((int)_length - (int)(front + back)) < 0)
            {
                set((value_type)0);
                _length = 0;
            }
            else
            {
                if (back > 0)
                {
                    /* zero out the newly hidden back and shrink the _length
                    this must be done before the front, otherwise the set would be offset */
                    set(_length - back, back, (value_type)0);
                    _length -= back;
                }
                if (front > 0)
                {
                    /* zero out the newly hidden front, move the _offset and shrink _length */
                    set(0, front, (value_type)0);
                    _offset += front;
                    _length -= front;
                }
            }
        }
        /* expand the container by other.size() bytes and copy other's contents into that space */
        void push_front(const bytes & other)
        {            
            expand(other.size(), 0);
            std::copy(other.begin(), other.end(), begin());
        }
        void push_front(const value_type b)
        {
            expand(1, 0);
            at(0) = b;
        }
        /* expand the container by other.size() bytes and copy other's contents into that space */
        void push_back(const bytes & other)
        {
            expand(0, other.size());
            std::copy(other.begin(), other.end(), end() - other.size());
        }
        void push_back(const bytes * other)
        {
            expand(0, other->size());
            std::copy(other->begin(), other->end(), end() - other->size());
        }
        void push_back(const value_type b)
        {
            expand(0, 1);
            at(size() - 1) = b;
        }

        bytes sub(const_iterator b, const_iterator e) const
        {
            bytes ret(e - b);
            std::copy(b, e, ret.begin());
            return ret;
        }
        bytes sub(size_type start, size_type length) const
        {
            return sub(begin() + start, begin() + start + length);
        }
        
        /* set all bytes to value */
        void set(value_type value)
        {
            for (uint i = 0; i < _length; i++)
                at(i) = value;
        }
        void set(size_type start, size_type length, value_type value)
        {
            length += start;
            for (size_type i = start; i < length; i++)
                at(i) = value;
        }
        /* safe to call multiple times, frees the resources for the HEAP type and sets up the
        container as if it was just initialized using the default constructor */
        void clear()
        {
            if (_data != nullptr)
                delete[] _data;
            
            _init();
        }
        /* releases the internally stored data buffer, use the get_offset function before calling
        this one in case capacity != size to obtain the the offset index, which indicates the 
        length of preallocated front */
        pointer release()
        {
            pointer ret = _data;
            _init();
            return ret;
        }
        
        /* used internally for move, do not use otherwise */
        void _init()
        {
            _data = nullptr;
            _length = 0;
            _offset = 0;
            _capacity = 0;
        }
        /* returns the number of actually allocated bytes */
        size_type capacity() const {return _capacity;}
        size_type get_offset() const {return _offset;}
        /* returns pointer to the beggining of the data */
        pointer get_base() const {return _data;}
        size_type _back() const {return _capacity - _offset - _length;}


        
        protected:
        pointer _data;
        size_type _length, _capacity, _offset;

        inline void range_check(size_type i) const
        {
            if (i >= _length || !_data)
                throw out_of_range("bytes::range_check at index " + std::to_string(i) + " (size " + std::to_string(_length) + ")");
        }
        void copy_from(const_pointer data, size_type length){
            if (!data || length == 0)
                return;
        
            range_check(length - 1);
            for (uint i = 0; i < length; i++)
                _data[i + _offset] = data[i];
        }
        void copy_to(pointer data, size_type length) const
        {
            if (!data || length == 0)
                return;
            
            range_check(length - 1);
            for (uint i = 0; i < length; i++)
                data[i] = _data[i + _offset];
        }
        /* replaces the _data with a newly allocated and initialized array of length, does not change the _capacity nor the _length! */
        void alloc(size_type length)
        {
            if (length > 0)
                _data = new value_type[length]();
            else 
                _data = nullptr;
        }
    };

    template<typename T>
    bytes to_bytes(const T& thing, bytes::size_type additional_capacity = 0)
    {
        bytes b(0, sizeof(thing), additional_capacity);
        std::copy(reinterpret_cast<const byte*>(&thing), reinterpret_cast<const bytes::value_type*>(&thing)
            + sizeof(thing), b.begin());
        return b;
    }
}

bool operator==(const sp::bytes & lhs, const sp::bytes & rhs)
{
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

bool operator!=(const sp::bytes & lhs, const sp::bytes & rhs)
{
    return !(lhs == rhs);
}

sp::bytes operator+(const sp::bytes & lhs, const sp::bytes & rhs)
{
    sp::bytes b(lhs.size() + rhs.size());
    std::copy(lhs.cbegin(), lhs.cend(), b.begin());
    std::copy(rhs.cbegin(), rhs.cend(), b.begin() + lhs.size());
    return b;
}

#ifndef SP_NO_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::bytes& obj) 
{
    os << "[ ";
    for (sp::bytes::size_type i = 0; i < obj.size(); i++)
        os << (int)obj[i] << ' ';
    return os << ']';
}
#endif


//sp::dynamic_bytes & operator+ (const sp::dynamic_bytes & lhs, const sp::dynamic_bytes rhs);

#endif


