#include <libprotoserial/container.hpp>








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