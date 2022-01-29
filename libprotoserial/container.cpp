#include <libprotoserial/container.hpp>

#include <string>
#include <stdexcept>

namespace sp
{
    bytes::bytes()
    {
        _init();
    }

    bytes::bytes(uint length): bytes(0, length, 0) {}

    bytes::bytes(uint front, uint length, uint back):
        bytes()
    {
        _capacity = front + length + back;
        _offset = front;
        _length = length;
        alloc(_capacity);
    }

    bytes::bytes(byte* data, uint length, alloc_t type)
    {
        _data = data;
        _capacity = _length = length;
        _type = type;
        _offset = 0;
    }

    bytes::~bytes()
    {
        clear();
    }


    bytes::bytes(const bytes & other) :
        bytes(other.size())
    {
        copy_from(other.data(), size());
    }

    bytes::bytes(bytes && other)
    {
        _data = other.data();
        _length = other.size();
        _type = other.get_type();
        _offset = other.get_offset();
        _capacity = other.get_capacity();
        other._init();
    }

    bytes & bytes::operator= (const bytes & other)
    {
        if (other.size() != size() || _type != HEAP) 
        {
            clear();
            alloc(other.size());
        }
        copy_from(other.data(), size());
        return *this;
    }

    bytes & bytes::operator= (bytes && other)
    {
        clear();
        _data = other.data();
        _length = other.size();
        _type = other.get_type();
        _offset = other.get_offset();
        _capacity = other.get_capacity();
        other._init();
        return *this;
    }



    uint bytes::size() const
    {
        return _length;
    }

    byte* bytes::data() const
    {
        return &_data[_offset];
    }


    bytes::alloc_t bytes::get_type() const
    {
        return _type;
    }

    uint bytes::get_capacity() const
    {
        return _capacity;
    }

    uint bytes::get_offset() const
    {
        return _offset;
    }

    byte* bytes::get_base() const
    {
        return _data;
    }


    byte & bytes::at(uint i)
    {
        i += _offset;
        range_check(i);
        return _data[i];
    }

    const byte & bytes::at(uint i) const
    {
        i += _offset;
        range_check(i);
        return _data[i];
    }

    byte & bytes::operator[] (uint i)
    {
        return at(i);
    }

    const byte & bytes::operator[] (uint i) const
    {
        return at(i);
    }


    void bytes::expand(uint front, uint back)
    {
        uint new_length, new_offset;
        byte *old_base;
        bytes::alloc_t old_type;
        bool reallocate = false;

        /* do nothing */
        if (front == 0 && back == 0)
            return;
        
        new_length = front + back + _length;

        if (front > _offset)
        {
            new_offset = 0;
            reallocate = true;
        }
        else
            new_offset = _offset - front;

        if (new_length > _capacity)
            reallocate = true;


        if (reallocate)
        {
            old_base = _data;
            old_type = _type;
            
            alloc(new_length);
            _capacity = new_length;

            if (old_base)
            {
                for (int i = 0; i < _length; i++)
                    _data[i + front] = old_base[i + _offset];
            
                if (old_type == HEAP)
                    delete[] old_base;
            }
        }
        
        _offset = new_offset;
        _length = new_length;
    }



    void bytes::set(byte value)
    {
        for (uint i = 0; i < _length; i++)
            _data[i + _offset] = value;
    }

    void bytes::set(uint start, uint length, byte value)
    {
        length += start;
        for (uint i = start; i < length; i++)
            _data[i + _offset] = value;
    }

    void bytes::clear()
    {
        if (_data != nullptr && _type == HEAP)
            delete[] _data;
        
        _init();
    }

    void bytes::_init()
    {
        _data = nullptr;
        _type = INIT;
        _length = 0;
        _offset = 0;
        _capacity = 0;
    }

    void bytes::to_heap()
    {
        byte *tmp = nullptr;
        
        if (_type == HEAP)
            return;

        tmp = _data;
        alloc(_length);
        copy_from(tmp, _length);
    }

    inline void bytes::range_check(uint i) const
    {
        if (i >= _length || !_data)
            throw std::out_of_range("at index " + std::to_string(i) + " (size " + std::to_string(_length) + ")");
    }

    void bytes::copy_from(const byte* data, uint length)
    {
        if (!data || length == 0)
            return;
        for (uint i = 0; i < length; i++)
            at(i) = data[i];
    }

    void bytes::copy_to(byte* data, uint length) const
    {
        if (!data || length == 0)
            return;
        for (uint i = 0; i < length; i++)
            data[i] = at(i);
    }

    void bytes::alloc(uint length)
    {
        if (length > 0)
            _data = new byte[length];
        else 
            _data = nullptr;
        _type = HEAP;
    }

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




/* namespace sp
{    
    static_bytes::static_bytes(byte* data, uint length)
    {
        _data = data;
        _length = length;
    }
}



namespace sp
{
    void dynamic_bytes::alloc(uint length)
    {
        if (length > 0)
            _data = new byte[length];
        _length = length;
    }

    dynamic_bytes::dynamic_bytes(uint length)
    {
        alloc(length);
    }

    void dynamic_bytes::clear()
    {
        if (_data != nullptr)
        {
            delete[] _data;
            _data = nullptr;
        }
        _length = 0;
    }

    dynamic_bytes::~dynamic_bytes()
    {
        clear();
    }

    dynamic_bytes::dynamic_bytes(const byte* data, uint length) : 
        dynamic_bytes(length)
    {
        copy_from(data, length);
    }

    dynamic_bytes::dynamic_bytes(const bytes & other) :
        dynamic_bytes(other.size())
    {
        copy_from(other.data(), size());
    }

    dynamic_bytes::dynamic_bytes(const dynamic_bytes & other) :
        dynamic_bytes(other.size())
    {
        copy_from(other.data(), size());
    }

    dynamic_bytes::dynamic_bytes(dynamic_bytes && other)
    {
        _data = other.data();
        _length = other.size();
        other.clear();
    }

    dynamic_bytes & dynamic_bytes::operator= (const bytes & other)
    {
        if (other.size() != size()) 
        {
            clear();
            alloc(other.size());
        }
        copy_from(other.data(), size());
        return *this;
    }

    dynamic_bytes & dynamic_bytes::operator= (dynamic_bytes && other)
    {
        clear();
        _data = other.data();
        _length = other.size();
        other.clear();
        return *this;
    }
}


 */