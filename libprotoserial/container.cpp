#include <libprotoserial/container.hpp>

#include <string>
#include <stdexcept>

namespace sp
{
    bytes::bytes()
    {
        _init();
    }

    bytes::bytes(uint length):
        bytes()
    {
        alloc(length);
    }

    bytes::bytes(byte* data, uint length, alloc_t type)
    {
        _data = data;
        _length = length;
        _type = type;
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
        _type = other.type();
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
        _type = other.type();
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

    bytes::alloc_t bytes::type() const
    {
        return _type;
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


    void bytes::expand(uint front, uint back, byte value)
    {



    }



    void bytes::set(byte value)
    {
        for (uint i = 0; i < _length; i++)
            _data[i] = value;
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
        _length = 0;
        _type = INIT;
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
        _length = length;
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