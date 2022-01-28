#include <libprotoserial/stream.hpp>

#include <iostream>

namespace sp
{
    void producer::bind(std::shared_ptr<consumer> c)
    {
        _consumer = c;
    }

    void producer::dispatch(std::unique_ptr<bytes> && data)
    {
        /* if (!_consumer->writable())
            throw consumer::not_writable(); */

        _consumer->write(std::move(data));
        
        /* size_t total = consumers.size(), i = 0;

        for (auto c : consumers)
        {
            i++;
            /* leave the last container for the last function call, 
            copy it for the preceding - usually we only have one callback
            if (i == total)
                c->write();
            else
                c->write(std::move(std::make_unique<bytes>(dynamic_bytes(*data))));
        }
        return i; */
    }
}


namespace sp
{
    int loopback_stream::write(std::unique_ptr<bytes> && data)
    {
        int len = data.get()->size();
        dispatch(std::move(data));
        return len;
    }

    bool loopback_stream::writable() 
    {
        return true;
    }

}



namespace sp
{
    int cout_stream::write(std::unique_ptr<bytes> && data)
    {
        int len = data.get()->size();
        std::cout << data.get()->data() << std::endl;
        //dispatch(std::move(data));
        return len;
    }

    bool cout_stream::writable() 
    {
        return true;
    }
}


