
//#define SP_BUFFERED_WARNING

#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>

#include "libprotoserial/interface.hpp"

#include "helpers/random.hpp"

using namespace std;
using namespace sp::literals;



int main(int argc, char const *argv[])
{

    sp::virtual_interface interface1(0, 1, 255, 10, 64, 1024), interface2(1, 2, 255, 10, 64, 1024);

    interface1.status_event.subscribe([](sp::interface::status s){
        cout << "I1: " << s << endl;
    });
    interface2.status_event.subscribe([](sp::interface::status s){
        cout << "I2: " << s << endl;
    });
    
    interface2.receive_event.subscribe([](sp::fragment f){
        cout << f << endl;
    });

    for (int i = 0; i < 5; i++)
    {
        interface1.write_noexcept(sp::fragment(2, sp::bytes(10)));
        auto data = interface1.get_serialized();

        for (auto b : data)
            interface2.put_single_serialized(b);
    }

    
    for (int i = 0; i < 5; i++)
        interface2.main_task();

    return 0;
}



