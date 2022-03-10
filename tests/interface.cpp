
#include <iostream>
#include <cstdlib>
#include <memory>

#include "libprotoserial/interface.hpp"

#include "helpers/random.hpp"

using namespace std;
using namespace sp::literals;

void test_interface(sp::interface & interface, uint loops, function<sp::bytes(void)> data_gen, 
    function<sp::interface::address_type(void)> addr_gen)
{
    std::unique_ptr<sp::interface::packet> tmp;
    uint i = 0;

    interface.receive_event.subscribe([&](sp::interface::packet p){
        if (*tmp != p) 
            cout << "loop: " << i << "\nORIG: " << *tmp << "\nGOT:  " << p << endl;
    });

    for (; i < loops; i++)
    {
        cout << i << endl;

        tmp.reset(new sp::interface::packet(addr_gen(), 1, data_gen(), &interface));
        
        interface.write(sp::interface::packet(*tmp));

        for (int j = 0; j < 3; j++)
            interface.main_task();
    }
}



int main(int argc, char const *argv[])
{
    sp::loopback_interface interface(0, 1, 10, 64, 256);

    auto data = [l = 0]() mutable {
        sp::bytes b(++l);
        std::generate(b.begin(), b.end(), [pos = 0]() mutable { return (sp::byte)(++pos); });
        return b;
    };
    auto addr = [&](){return (sp::interface::address_type)2;};

    test_interface(interface, 10, data, addr);

    //packet that passed the 32bit CRC but didn't match the TXed:  1 23 5 29 133 0 115 5 41   255 131 8 122
    
    
    /* sp::loopback_interface interface(0, 1, 10, 64, 1024, [](sp::byte b){
        if (chance(1)) b |= random_byte();
        return b;
    }); */

    /* std::unique_ptr<sp::interface::packet> tmp;

    interface.receive_event.subscribe([&](sp::interface::packet p){
        cout << "packed_received_event" << endl;
        cout << "  dst: " << p.destination() << "  src: " << p.source() << endl;
        cout << "  interface: " << p.interface()->name() << endl;
        cout << "  data: " << p.data() << endl;
        auto& o = *tmp;
        if (o != p) 
        {
            cout << "  PACKETS DO NOT MATCH - ORIGINAL" << endl;
            cout << "  dst: " << o.destination() << "  src: " << o.source() << endl;
            cout << "  interface: " << o.interface()->name() << endl;
            cout << "  data: " << o.data() << endl;
        }
    }); */


    /* for (int i = 0; i < 11; i++)
    {
        //if (i % 10 == 0)
            cout << i << endl;
        

        tmp.reset(new sp::interface::packet(random(2, 100), 1, 
            random_bytes(1, interface.max_data_size()), &interface));
        
        interface.write(sp::interface::packet(*tmp));


        try
        {
            for (int j = 0; j < 3; j++)
                interface.main_task();
        }
        catch(std::exception &e)
        {
            std::cerr << "main exception: " << e.what() << '\n';
        }
        
    } */
    

    return 0;
}


