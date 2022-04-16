
#define SP_FRAGMENTATION_DEBUG
//#define SP_LOOPBACK_DEBUG
//#define SP_BUFFERED_DEBUG


#include "libprotoserial/fragmentation.hpp"
#include "libprotoserial/protostacks.hpp"

#include <iostream>
#include <thread>

using namespace std;
using namespace std::chrono_literals;
using namespace sp::literals;

template<typename Interface>
class handler
{
    Interface &src, &dst;
    sp::bytes d;
    sp::bytes::iterator i;

    public:

    handler(Interface & src, Interface & dst):
        src(src), dst(dst) {}

    void update()
    {
        if (d)
        {
            if (i == d.end())
                d = sp::bytes();
            else
            {
                dst.put_single_serialized(*i);
                ++i;
            }
        }
        else
        {
            if (src.has_serialized())
            {
                d = src.get_serialized();
                i = d.begin();
            }
        }
    }
};

int main(int argc, char const *argv[])
{
    sp::stack::virtual_full s1(0, 1, 1000), s2(1, 2, 1000);
    //sp::stack::loopback s1(0, 1, 1000);

    s1.fragmentation.transfer_receive_event.subscribe([](sp::transfer t){
        cout << "RX s1: " << t << endl;
    });
    s1.fragmentation.transfer_ack_event.subscribe([](sp::transfer_metadata tm){
        cout << "ACK s1: " << (int)tm.get_id() << endl;
    });
    s2.fragmentation.transfer_receive_event.subscribe([](sp::transfer t){
        cout << "RX s2: " << t << endl;
    });
    s2.fragmentation.transfer_ack_event.subscribe([](sp::transfer_metadata tm){
        cout << "ACK s2: " << (int)tm.get_id() << endl;
    });

    /* s1.interface.status_event.subscribe([](sp::interface::status s){
        cout << "s1: status: " << s << endl;
    });
    s1.interface.status_event.subscribe([](sp::interface::status s){
        cout << "s2: status: " << s << endl;
    }); */

    auto t = sp::transfer(s1.interface);
    t.set_destination(2);
    t.push_back(sp::bytes(100));
    s1.fragmentation.transmit(std::move(t));

    handler h1(s1.interface, s2.interface), h2(s2.interface, s1.interface);

    for (int i = 0; i < 100; i++)
    {
        if (!(i % 10))
            cout << "start " << i << endl;

        s1.main_task();
        s2.main_task();

        for (int j = 0; j < 10; j++)
        {
            h1.update();
            h2.update();
            std::this_thread::sleep_for(1ms);
        }
    }

    /* for (int i = 0; i < 5; i++)
    {
        s1.main_task();
        s2.main_task();
        std::this_thread::sleep_for(1ms);
    } */

    

    return 0;
}
