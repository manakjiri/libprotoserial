
#include <libprotoserial/interface.hpp>
#include <libprotoserial/fragmentation.hpp>
#include "gtest/gtest.h"
#include <functional>
#include <iostream>
#include <map>

using namespace std;

uint test_interface(sp::interface & interface, uint loops, function<sp::bytes(void)> data_gen, 
    function<sp::interface::address_type(void)> addr_gen)
{
    std::unique_ptr<sp::fragment> tmp;
    uint i = 0, received = 0;

    interface.receive_event.subscribe([&](sp::fragment p){
        //cout << "receive_event: " << p << endl;
        EXPECT_TRUE(tmp->data() == p.data() && tmp->destination() == p.source()) << "loop: " << i << "\nORIG: " << *tmp << "\nGOT:  " << p << endl;
        if (tmp->data() != p.data())
            cout << "here" << endl; // breakpoint hook
        received++;
    });

    for (; i < loops; i++)
    {
#ifdef SP_LOOPBACK_DEBUG
        cout << i << endl;
#endif
        tmp.reset(new sp::fragment(addr_gen(), data_gen()));
        
        interface.transmit(sp::fragment(*tmp));

        for (int j = 0; j < 3; j++)
            interface.main_task();
    }
    cout << "received " << received << " out of " << loops << " sent (" << (100.0 * received) / loops << "%)" << endl;
    return received;
}


uint test_handler(sp::interface & interface, sp::fragmentation_handler & handler, uint loops, 
    function<sp::bytes(void)> data_gen, function<sp::interface::address_type(void)> addr_gen, uint runs = 5)
{
    map<sp::transfer::id_type, tuple<sp::bytes, uint>> check;
    sp::bytes tmp;
    uint i = 0, received = 0;

    handler.transfer_receive_event.subscribe([&](sp::transfer t){
#ifdef SP_FRAGMENTATION_DEBUG
        cout << "receive_event: " << t << endl;
#endif
        auto b = t.data();
        tmp = get<0>(check[t.get_id()]);
        get<1>(check[t.get_id()])++;
        EXPECT_TRUE(tmp == b) << "loop: " << i << "\nORIG: " << tmp << "\nGOT:  " << t << endl;
        if (tmp != b)
            cout << "here" << endl; // breakpoint hook
        
        received++;
    });

    for (; i < loops; i++)
    {        
        sp::transfer t(interface.interface_id(), addr_gen());
        check[t.get_id()] = tuple(data_gen(), 0);
        t.data().push_back(sp::bytes(get<0>(check[t.get_id()])));
        handler.transmit(t);

        if (i == loops-1) runs *= 10;
        
        for (uint j = 0; j < runs; j++)
        {
            auto s = sp::clock::now();
#ifdef SP_FRAGMENTATION_DEBUG
            cout << "loop " << i << " run " << j << endl;
#endif
            interface.main_task();
            handler.main_task();
            while (!sp::older_than(s, 1ms)) {}
        }
    }
#ifdef SP_FRAGMENTATION_DEBUG
    handler.print_debug();
#endif
    EXPECT_EQ(check.size(), loops);
    cout << "received " << received << " out of " << loops << " sent (" << (100.0 * received) / loops << "%)" << endl;

    string s;
    for (const auto & [key, value] : check)
    {
        if (get<1>(value) != 1)
            s += to_string((int)key) + "[" + to_string(get<1>(value)) + "] ";
    }
    if (!s.empty())
        cout << "unreceived/duplicated IDs: " << s << endl;

    return received;
}
