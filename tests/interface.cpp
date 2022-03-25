
#define SP_BUFFERED_WARNING

#include <iostream>
#include <cstdlib>
#include <memory>
#include <thread>

#include "libprotoserial/interface.hpp"

#include "helpers/random.hpp"

using namespace std;
using namespace sp::literals;


void receive_handler(int * run, sp::interface * interface)
{
    while (*run)
    {
        interface->main_task();
        this_thread::sleep_for(1ns * random(10, 100));
    }
}

int main(int argc, char const *argv[])
{
    sp::virtual_interface interface1(0, 1, 10, 64, 256), interface2(1, 2, 10, 64, 48);

    interface2.receive_event.subscribe([](sp::fragment f){
        cout << "RX: " << f << endl;
    });
    
    //int run = true;
    //std::jthread receive(receive_handler, &run, &interface2);

    auto serialize = [&](sp::fragment f) {
        interface1.write(std::move(f));
        while (!interface1.has_serialized()) {cout << "!has_serialized()" << endl;}
        return interface1.get_serialized();
    };


    for (int i = 0; i < 3; i++)
    {
        sp::bytes d(10); //random(1, interface1.max_data_size())
        d.set((sp::byte)i);
        auto data = serialize(sp::fragment(2, std::move(d)));

        for (auto b : data)
            interface2.put_single_serialized(b);
        
        cout << interface2._get_rx_buffer() << endl;
    }
    
    for (int i = 0; i < 5; i++)
        interface2.main_task();


    auto data = serialize(sp::fragment(2, random_bytes(1, 5)));
    for (auto b : data)
            interface2.put_single_serialized(b);

    interface2.main_task();

    //this_thread::sleep_for(1000ms);
    //run = false;

    return 0;
}



