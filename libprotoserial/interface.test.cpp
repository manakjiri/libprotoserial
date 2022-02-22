
#include <iostream>
#include <cstdlib>

#include "libprotoserial/loopback_interface.hpp"

using namespace std;
using namespace sp::byte_literal;

sp::bytes random_bytes(sp::bytes::size_type size)
{
    sp::bytes b(size);
    std::generate(b.begin(), b.end(), [](){return std::rand();});
    return b;    
}


int main(int argc, char const *argv[])
{
    sp::loopback interface(0, 1, 10, 64, [](sp::byte b){
        static int c = 1;
        if (++c > 10) c = 0;
        return !c ? (b | 42_B) : b;
    });

    interface.packed_rxed_event.subscribe([](sp::interface::packet p){
        cout << "packed_received_event" << endl;
        cout << "  dst: " << p.destination() << "  src: " << p.source() << endl;
        cout << "  interface: " << p.interface()->name() << endl;
        cout << "  data: " << *p.data() << endl;
    });

    interface.write(sp::interface::packet(2, 1, sp::bytes({10_B, 11_B, 12_B})));
    interface.write(sp::interface::packet(3, 1, sp::bytes({20_B, 21_B, 22_B})));
    interface.write(sp::interface::packet(4, 1, sp::bytes({30_B, 31_B, 32_B})));

    for (int i = 0; i < 5; i++)
    {
        try
        {
            cout << i << endl;
            interface.main_task();
        }
        catch(std::exception &e)
        {
            std::cerr << "main exception: " << e.what() << '\n';
        }
        
    }
    

    return 0;
}

