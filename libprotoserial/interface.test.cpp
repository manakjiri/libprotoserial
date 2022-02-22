
#include <iostream>
#include <cstdlib>

#include "libprotoserial/loopback_interface.hpp"

using namespace std;
using namespace sp::byte_literal;

uint random(uint from, uint to)
{
    return from + (std::rand() % (to - from + 1));
}

bool chance(uint percent)
{
    return random(1, 100) <= percent;
}

sp::byte random_byte()
{
    return (sp::byte)std::rand();
}

sp::bytes random_bytes(sp::bytes::size_type size)
{
    sp::bytes b(size);
    std::generate(b.begin(), b.end(), random_byte);
    return b;    
}

sp::bytes random_bytes(uint from, uint to)
{
    return random_bytes(random(from, to));
}


int main(int argc, char const *argv[])
{
    sp::loopback interface(0, 1, 10, 64, [](sp::byte b){
        if (chance(1)) b |= random_byte();
        return b;
    });

    std::unique_ptr<sp::interface::packet> tmp;

    interface.packed_rxed_event.subscribe([&](sp::interface::packet p){
        auto& o = *tmp;
        if (o != p) 
        {
            cout << "packed_received_event" << endl;
            cout << "  dst: " << p.destination() << "  src: " << p.source() << endl;
            cout << "  interface: " << p.interface()->name() << endl;
            cout << "  data: " << p.data() << endl;
            cout << "  PACKETS DO NOT MATCH - ORIGINAL" << endl;
            cout << "  dst: " << o.destination() << "  src: " << o.source() << endl;
            cout << "  interface: " << o.interface()->name() << endl;
            cout << "  data: " << o.data() << endl;
        }
    });

    /* interface.write(sp::interface::packet(2, 1, sp::bytes({10_B, 11_B, 12_B})));
    interface.write(sp::interface::packet(3, 1, sp::bytes({20_B, 21_B, 22_B})));
    interface.write(sp::interface::packet(4, 1, sp::bytes({30_B, 31_B, 32_B}))); */

    for (int i = 0; i < 300; i++)
    {
        if (i % 10 == 0)
            cout << i << endl;
        
        /* if (i == 39) 
            cout << "now" << endl; */

        tmp.reset(new sp::interface::packet(random(2, 100), 1, 
            random_bytes(1, interface.max_data_size()), &interface));
        
        interface.write(sp::interface::packet(*tmp));


        try
        {
            interface.main_task();
        }
        catch(std::exception &e)
        {
            std::cerr << "main exception: " << e.what() << '\n';
        }
        
    }
    

    return 0;
}

