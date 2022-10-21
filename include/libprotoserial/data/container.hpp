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


#ifndef _SP_CONTAINER
#define _SP_CONTAINER

#include <libprotoserial/libconfig.hpp>
#include <libprotoserial/data/byte.hpp>

#include <initializer_list>
#include <string>
#include <algorithm>
#include <memory>
#include <cstring>

#ifdef SP_ENABLE_EXCEPTIONS
#include <stdexcept>
#endif

#ifdef SP_ENABLE_IOSTREAM
#include <iostream>
#endif


namespace sp
{
    using uint = unsigned int;
    using out_of_range = std::out_of_range;

    class bytes 
    {
        public:

        using value_type        = sp::byte;
        using size_type         = std::size_t;
        using difference_type   = std::ptrdiff_t;
        using pointer           = value_type *;
        using const_pointer     = const value_type *;
        using reference         = value_type &;
        using const_reference   = const value_type &;
        using iterator          = pointer;
        using const_iterator    = const_pointer;

        using allocator_type = std::allocator<value_type>;

        /* default */
        constexpr bytes()
        {
            _init(); 
        }
        constexpr bytes(size_type length) : bytes(0, length, 0) {}
        /* overallocation - capacity will be equal to front + length + back */
        constexpr bytes(size_type front, size_type length, size_type back) :
            bytes()
        {
            _capacity = front + length + back;
            _offset = front;
            _length = length;
            alloc(_capacity);
        }
        /* use this if you want to wrap existing raw array of bytes by this class,
        the container takes ownership and will free the provided pointer using allocator deallocate */
        constexpr bytes(pointer data, size_type length) :
            bytes()
        {
            _data = data;
            _length = length;
        }

        constexpr bytes(std::initializer_list<value_type> values):
            bytes(values.size())
        {
            std::copy(values.begin(), values.end(), begin());
        }

        template<typename OutputIt>
        constexpr bytes(const OutputIt & begin, const OutputIt & end):
            bytes()
        {
            std::copy(begin, end, std::back_inserter(*this));
        }

        bytes(const std::string & from) :
            bytes(from.size())
        {
            copy_from(reinterpret_cast<bytes::pointer>(const_cast<char*>(from.c_str())), from.size());
        }        
        
        /* copy - only the currently exposed data gets copied, overallocation is not used */
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
        constexpr bytes(bytes && other)
        {
            _data = other.get_base();
            _length = other.size();
            _offset = other.capacity_front();
            _capacity = other.capacity();
            other._init();
        }
        constexpr bytes & operator= (bytes && other)
        {
            clear();
            _data = other.get_base();
            _length = other.size();
            _offset = other.capacity_front();
            _capacity = other.capacity();
            other._init();
            return *this;
        }

        constexpr ~bytes()
        {
            clear();
        }
        
        
        /* returns the current data size, if overallocation is not used than size == capacity */
        constexpr size_type size() const {return _length;}
        /* a container is empty when its size is equal to 0 */
        constexpr bool is_empty() const {return size() == 0;}
        /* returns pointer to data */
        constexpr pointer data()
        {
            if (_data)
                return &_data[_offset];
            else
                return nullptr;
        }
        constexpr const_pointer data() const
        {
            if (_data)
                return &_data[_offset];
            else
                return nullptr;
        }
        
        constexpr const value_type & at(size_type i) const
        {
            range_check(i);
            return data()[i];
        }
        constexpr value_type & at(size_type i)
        {
            range_check(i);
            return data()[i];
        }
        const value_type & operator[] (size_type i) const {return at(i);}
        value_type & operator[] (size_type i) {return at(i);}

        constexpr iterator begin() {return iterator(data());}
        constexpr iterator end() {return iterator(data() + size());}
        constexpr const_iterator begin() const {return const_iterator(data());}
        constexpr const_iterator end() const {return const_iterator(data() + size());}
        constexpr const_iterator cbegin() const {return const_iterator(data());}
        constexpr const_iterator cend() const {return const_iterator(data() + size());}

        constexpr explicit operator bool() const {return !is_empty();}
        
        /* expands the container by the requested amount such that [front B][size B][back B], 
        front or back can be 0, in which case nothing happens */
        constexpr void expand(const size_type front, const size_type back)
        {
            reserve(front, back);
            _offset = _offset - front;
            _length = _length + front + back;
        }
        /* capacity of the container will be equal or greater than size() + front + back, size() does not change,
        this function merely reserves requested capacity by reallocation if necesary, front or back can be 0 */
        constexpr void reserve(const size_type front, const size_type back)
        {
            using traits_t = std::allocator_traits<allocator_type>;

            /* do nothing if the container has enough margin already */
            if (_offset >= front && capacity_back() >= back)
                return;

            /* keep pointer to the old buffer since we need to reallocate it */
            pointer old_data = _data;
            size_type old_capacity = _capacity;
            
            /* allocate the new data buffer and update the capacity so it reflects this */
            _capacity = front + _length + back;
            alloc(_capacity);

            if (old_data)
            {
                /* copy the original data */
                for (size_type i = 0; i < _length; i++)
                    _data[i + front] = old_data[i + _offset];

                traits_t::deallocate(_alloc, old_data, old_capacity);
            }

            /* finally update the offset because we no longer need the old value */
            _offset = front;
        }
        /* shrink the container from either side, this does not reallocate the data, just hides it
        use the shrink_to_fit function after this one to actually reduce the container size */
        constexpr void shrink(const size_type front, const size_type back)
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
        constexpr void push_front(const bytes & other)
        {            
            expand(other.size(), 0);
            std::copy(other.begin(), other.end(), begin());
        }
        constexpr void push_front(const value_type b)
        {
            expand(1, 0);
            at(0) = b;
        }
        /* expand the container by other.size() bytes and copy other's contents into that space */
        constexpr void push_back(const bytes & other)
        {
            expand(0, other.size());
            std::copy(other.begin(), other.end(), end() - other.size());
        }
        constexpr void push_back(const value_type & b)
        {
            expand(0, 1);
            at(size() - 1) = b;
        }
        constexpr void push_back(const value_type && b)
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
        constexpr void set(value_type value)
        {
            for (uint i = 0; i < _length; i++)
                at(i) = value;
        }
        constexpr void set(size_type start, size_type length, value_type value)
        {
            length += start;
            for (size_type i = start; i < length; i++)
                at(i) = value;
        }
        /* safe to call multiple times, frees the resources for the HEAP type and sets up the
        container as if it was just initialized using the default constructor */
        constexpr void clear()
        {
            using traits_t = std::allocator_traits<allocator_type>;

            if (_data && _capacity != 0)
                traits_t::deallocate(_alloc, _data, _capacity);
            
            _init();
        }
        /* releases the internally stored data buffer, use the capacity_front function before calling
        this one in case capacity != size to obtain the the offset index, which indicates the 
        length of preallocated front */
        constexpr pointer release()
        {
            pointer ret = _data;
            _init();
            return ret;
        }
        
        /* used internally for move, do not use otherwise */
        constexpr void _init()
        {
            _data = pointer();
            _length = 0;
            _offset = 0;
            _capacity = 0;
        }
        /* returns the number of actually allocated bytes */
        constexpr size_type capacity() const {return _capacity;}
        constexpr size_type capacity_front() const {return _offset;}
        constexpr size_type capacity_back() const {return _capacity - _offset - _length;}
        /* returns pointer to the beggining of the data */
        constexpr pointer get_base() const {return _data;}


        
        protected:
        pointer _data;
        size_type _length, _capacity, _offset;
        allocator_type _alloc;

        constexpr inline void range_check(size_type i) const
        {
            if (i >= _length || !_data)
            {
#ifdef SP_ENABLE_EXCEPTIONS
                throw out_of_range("bytes::range_check at index " + std::to_string(i) + " (size " + std::to_string(_length) + ")");
#else
                std::terminate();
#endif
            }
        }
        void copy_from(const_pointer data, const size_type length)
        {
            if (!data || length == 0)
                return;
        
            range_check(length - 1);
            std::memcpy(&_data[_offset], data, length);
        }
        void copy_to(pointer data, const size_type length) const
        {
            if (!data || length == 0)
                return;
            
            range_check(length - 1);
            std::memcpy(data, &_data[_offset], length);
        }
        /* replaces the _data with a newly allocated and initialized array of length, does not change the _capacity nor the _length! */
        constexpr void alloc(size_type length)
        {
            if (length > 0)
            {
                using traits_t = std::allocator_traits<allocator_type>;
                _data = traits_t::allocate(_alloc, length);
                std::memset(_data, 0, length);
            }
            else 
                _data = pointer();
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

#ifdef SP_ENABLE_IOSTREAM
std::ostream& operator<<(std::ostream& os, const sp::bytes& obj) 
{
    os << "[ ";

#ifndef SP_ENABLE_FORMAT
    std::ios_base::fmtflags f(os.flags());
    os << std::hex;
#endif

    for (sp::bytes::size_type i = 0; i < obj.size(); i++)
    {
#ifdef SP_ENABLE_FORMAT
        os << format("{:#04x}\n", (int)obj[i]) << ' ';
#else
        os << std::setfill('0') << std::setw(2) << std::right << (int)obj[i] << ' ';
#endif
    }

#ifndef SP_ENABLE_FORMAT
    os.flags(f);
#endif

    return os << ']';
}
#endif


#endif


