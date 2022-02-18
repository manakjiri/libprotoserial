/*
 * protocols need a byte container, which not only can reserve space in the back, but
 * also, perhaps more importantly, at the front of the data
 * 
 * 
 */



#include <libprotoserial/byte.hpp>

#include <initializer_list>
#include <string>

namespace sp
{
    using uint = unsigned int;

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

        //bytes(std::initializer_list<byte> values);
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
            _data = other._base();
            _length = other.size();
            _offset = other._shift();
            _capacity = other.capacity();
            other._init();
        }
        bytes & operator= (bytes && other)
        {
            clear();
            _data = other._base();
            _length = other.size();
            _offset = other._shift();
            _capacity = other.capacity();
            other._init();
            return *this;
        }

        ~bytes()
        {
            clear();
        }
        
        
        /* returns the current data size, if overallocation is not used than size == capacity */
        constexpr size_type size() const {return _length;}
        /* returns pointer to data */
        constexpr pointer data() const
        {
            if (_data)
                return &_data[_offset];
            else
                return nullptr;
        }
        
        constexpr const byte & at(size_type i) const
        {
            range_check(i);
            return _data[i + _offset];
        }
        constexpr byte & at(size_type i)
        {
            range_check(i);
            return _data[i + _offset];
        }
        constexpr const byte & operator[] (size_type i) const {return at(i);}
        constexpr byte & operator[] (size_type i) {return at(i);}

        constexpr iterator begin() {return &_data[_offset];}
        constexpr iterator end() {return &_data[_offset + _length];}
        constexpr const_iterator begin() const {return &_data[_offset];}
        constexpr const_iterator end() const {return &_data[_offset + _length];}
        constexpr const_iterator cbegin() const {return &_data[_offset];}
        constexpr const_iterator cend() const {return &_data[_offset + _length];}
        
        /* expands it by the requested amount such that [front B][size B][back B], front or back can be 0 */
        void expand(size_type front, size_type back)
        {
            size_type new_length, new_offset;
            pointer old_base;
            bool reallocate = false;

            /* do nothing */
            if (front == 0 && back == 0)
                return;
            
            /* this will be out now size() */
            new_length = front + back + _length;

            /* _offset says how many overallocated "bumper" bytes we have, so if the requested
            front is more than that, we know the data needs to be expanded */
            if (front > _offset)
            {
                new_offset = 0;
                reallocate = true;
            }
            /* if we have enough bytes at the front we may not need to reallocate, just shift the offset */
            else
                new_offset = _offset - front;
            
            /* if the total requested length is over the capacity, we obviously need to allocate a larger buffer */
            if (new_length > _capacity)
                reallocate = true;

            if (reallocate)
            {
                /* keep reference to the old buffer and take note whether we need to delete it */
                old_base = _data;
                
                /* allocate the new data buffer and update the capacity so it refelects this */
                alloc(new_length);
                _capacity = new_length;

                if (old_base)
                {
                    /* copy the original data */
                    for (uint i = 0; i < _length; i++)
                        _data[i + front] = old_base[i + _offset];
                
                    delete[] old_base;
                }
            }
            
            /* finally update the offset and length because we no longer need the old values */
            _offset = new_offset;
            _length = new_length;
        }
        /* expand the container by other.size() bytes and copy other's contents into that space */
        void prepend(const_reference other);
        void append(const_reference other);
        
        /* set all bytes to value */
        constexpr void set(byte value)
        {
            for (uint i = 0; i < _length; i++)
                _data[i + _offset] = value;
        }
        constexpr void set(size_type start, size_type length, byte value)
        {
            length += start;
            for (uint i = start; i < length; i++)
                _data[i + _offset] = value;
        }
        /* safe to call multiple times, frees the resources for the HEAP type and sets up the
        container as if it was just initialized using the default constructor */
        constexpr void clear()
        {
            if (_data != nullptr)
                delete[] _data;
            
            _init();
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
        constexpr size_type capacity() const {return _capacity;}
        constexpr size_type _shift() const {return _offset;}
        /* returns pointer to the beggining of the data */
        constexpr pointer _base() const {return _data;}


        
        protected:
        pointer _data;
        size_type _length, _capacity, _offset;

        inline void range_check(size_type i) const
        {
            if (i >= _length || !_data)
                throw std::out_of_range("at index " + std::to_string(i) + " (size " + std::to_string(_length) + ")");
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
                _data = new byte[length]();
            else 
                _data = nullptr;
        }
    };
}

bool operator==(const sp::bytes & lhs, const sp::bytes rhs)
{
    uint size;
    if ((size = lhs.size()) != rhs.size()) 
        return false;
    
    for (uint i = 0; i < size; i++)
    {
        if (lhs.at(i) != rhs.at(i))
            return false;
    }
    return true;
}

bool operator!=(const sp::bytes & lhs, const sp::bytes rhs)
{
    return !(lhs == rhs);
}


//sp::dynamic_bytes & operator+ (const sp::dynamic_bytes & lhs, const sp::dynamic_bytes rhs);


