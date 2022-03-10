


#include "libprotoserial/fragmentation.hpp"

#include <iostream>

using namespace std;
using namespace std::chrono_literals;
using namespace sp::literals;

int main(int argc, char const *argv[])
{
    sp::loopback_interface interface(0, 1, 10, 32, 256);
    sp::fragmentation_handler handler(interface.max_data_size(), 10ms, 100ms, 5);

    interface.receive_event.subscribe(&sp::fragmentation_handler::receive_callback, &handler);
    handler.transmit_event.subscribe(&sp::loopback_interface::write_noexcept, 
        reinterpret_cast<sp::interface*>(&interface));

    handler.transfer_receive_event.subscribe([](sp::transfer t){
        cout << "transfer_receive_event" << endl;
        cout << t << endl;
    });

    auto t = handler.new_transfer();
    t.set_destination(2);
    for (int i = 0; i < 20; ++i)
        t.push_back({10_B, 11_B, 12_B, 13_B, 14_B});
    t.push_front({30_B, 31_B});
    handler.transmit(t);
    

    for (int i = 0; i < 10; i++)
    {
        interface.main_task();
        handler.main_task();
    }
    
    

    return 0;
}
