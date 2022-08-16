
#include <libprotoserial/interface.hpp>
#include <libprotoserial/fragmentation.hpp>
#include <libprotoserial/ports/packet.hpp>
//#include <libprotoserial/protostacks.hpp>

#include "helpers/random.hpp"
#include "helpers/testers.hpp"

#include <map>
#include <tuple>

#include "gtest/gtest.h"

using namespace std;
using namespace std::chrono_literals;
using namespace sp::literals;

#define SP_LOOPBACK_DEBUG


TEST(Bytes, Constructor) 
{
    sp::bytes b1(10);
    EXPECT_TRUE(b1.data() != nullptr);
    EXPECT_EQ(10, b1.size());

    sp::bytes b2(2, 10, 1);
    EXPECT_TRUE(b2.data() != nullptr);
    EXPECT_EQ(10, b2.size());
    EXPECT_TRUE(13 >= b2.capacity());

    sp::bytes b3;
    EXPECT_TRUE(b3.data() == nullptr);
    EXPECT_EQ(0, b3.size());
    EXPECT_EQ(0, b3.capacity());

    sp::bytes b4 = {1_BYTE, 10_BYTE, 25_BYTE};
    EXPECT_TRUE(b4.size() == 3);
    EXPECT_TRUE(b4[0] == 1_BYTE);
    EXPECT_TRUE(b4[1] == 10_BYTE);
    EXPECT_TRUE(b4[2] == 25_BYTE);

}

TEST(Bytes, Access)
{
    sp::bytes b1(3);
    sp::bytes b2(2, 3, 5);

    for (uint i = 0; i < b1.size(); i++)
        b1[i] = b2[i] = (sp::byte)(i + 10);

    for (uint i = 0; i < b1.size(); i++)
    {
        EXPECT_EQ(b1[i], (sp::byte)(i + 10));
        EXPECT_EQ(b2[i], (sp::byte)(i + 10));
    }

    EXPECT_THROW(b1[4], sp::out_of_range);
    EXPECT_THROW(b2[4], sp::out_of_range);
}

TEST(Bytes, Copy)
{
    sp::bytes b1(3);
    sp::bytes b2(30, 3, 20);

    for (uint i = 0; i < b1.size(); i++)
        b1[i] = b2[i] = (sp::byte)(i + 20);

    EXPECT_TRUE(b1 == b2);
    
    sp::bytes b3 = b1;
    EXPECT_TRUE(b1 == b3);
    EXPECT_TRUE(b2 == b3);
    EXPECT_TRUE(b1.data() != b3.data());

    sp::bytes b4(b1);
    EXPECT_TRUE(b1 == b4);
    EXPECT_TRUE(b2 == b4);
    EXPECT_TRUE(b1.data() != b4.data());

    sp::bytes b5 = b2;
    EXPECT_TRUE(b1 == b5);
    EXPECT_TRUE(b2 == b5);
    EXPECT_TRUE(b2.data() != b5.data());

    sp::bytes b6(b2);
    EXPECT_TRUE(b1 == b6);
    EXPECT_TRUE(b2 == b6);
    EXPECT_TRUE(b2.data() != b6.data());
}

TEST(Bytes, Move)
{
    sp::bytes b1(3);
    sp::bytes b2(4, 3, 10);

    for (uint i = 0; i < b1.size(); i++)
        b2[i] = b1[i] = (sp::byte)(i + 30);

    sp::bytes::pointer pb1 = b1.data(), pb2 = b2.data();

    auto check = [&](const sp::bytes & p, const sp::bytes & n, 
    sp::bytes::size_type prev_size, sp::bytes::size_type prev_capacity, 
    sp::bytes::pointer pointer) {
        EXPECT_TRUE(p.size() == 0);
        EXPECT_TRUE(p.capacity() == 0);
        EXPECT_TRUE(p.data() == nullptr);
        EXPECT_TRUE(n.size() == prev_size);
        EXPECT_TRUE(n.capacity() == prev_capacity);
        EXPECT_TRUE(n.data() == pointer);
    };

    sp::bytes b3 = std::move(b1);
    sp::bytes b4 = std::move(b2);

    check(b1, b3, 3, 3, pb1);
    check(b2, b4, 3, 17, pb2);

    for (uint i = 0; i < b1.size(); i++)
    {
        EXPECT_TRUE(b3[i] == (sp::byte)(i + 30));
        EXPECT_TRUE(b4[i] == (sp::byte)(i + 30));
    }
}

TEST(Bytes, Set)
{
    sp::bytes b1(5);
    sp::bytes b2(4, 5, 3);
    sp::bytes bc;

    bc = {0_BYTE, 0_BYTE, 0_BYTE, 0_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {255_BYTE, 255_BYTE, 255_BYTE, 255_BYTE, 255_BYTE};
    b1.set(255_BYTE);
    b2.set(255_BYTE);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_BYTE, 2_BYTE, 255_BYTE, 255_BYTE, 255_BYTE};
    b1.set(0, 2, 2_BYTE);
    b2.set(0, 2, 2_BYTE);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_BYTE, 2_BYTE, 3_BYTE, 3_BYTE, 3_BYTE};
    b1.set(2, 3, 3_BYTE);
    b2.set(2, 3, 3_BYTE);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_BYTE, 4_BYTE, 4_BYTE, 4_BYTE, 3_BYTE};
    b1.set(1, 3, 4_BYTE);
    b2.set(1, 3, 4_BYTE);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    EXPECT_THROW(b1.set(0, 6, 10_BYTE), sp::out_of_range);
    EXPECT_THROW(b2.set(0, 6, 10_BYTE), sp::out_of_range);
    EXPECT_THROW(b1.set(3, 6, 11_BYTE), sp::out_of_range);
    EXPECT_THROW(b2.set(3, 6, 11_BYTE), sp::out_of_range);

    bc = {10_BYTE, 10_BYTE, 10_BYTE, 11_BYTE, 11_BYTE};
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;
}

TEST(Bytes, Expand)
{
    const sp::bytes bo = {1_BYTE, 2_BYTE, 3_BYTE};
    std::vector<std::unique_ptr<sp::bytes>> vb;
    vb.push_back(std::make_unique<sp::bytes>(3));
    vb.push_back(std::make_unique<sp::bytes>(2, 3, 0));
    vb.push_back(std::make_unique<sp::bytes>(0, 3, 2));
    vb.push_back(std::make_unique<sp::bytes>(2, 3, 2));
    sp::bytes::pointer pb3 = vb.at(3)->get_base();

    for (auto& b : vb)
    {
        sp::bytes bc = bo;
        
        std::copy(bo.begin(), bo.end(), b->begin());
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;

        b->expand(1, 0);
        bc = {0_BYTE, 1_BYTE, 2_BYTE, 3_BYTE};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
        b->at(0) = 10_BYTE;

        b->expand(0, 1);
        bc = {10_BYTE, 1_BYTE, 2_BYTE, 3_BYTE, 0_BYTE};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
        b->at(4) = 20_BYTE;

        b->expand(1, 1);
        bc = {0_BYTE, 10_BYTE, 1_BYTE, 2_BYTE, 3_BYTE, 20_BYTE, 0_BYTE};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
    }

    EXPECT_TRUE(pb3 == vb.at(3)->get_base()) << "at(3) should not relocate";

    sp::bytes b1, bc;
    b1.expand(1, 0);
    b1[0] = 1_BYTE;
    b1.expand(0, 1);
    b1[1] = 2_BYTE;
    b1.expand(1, 0);
    b1[0] = 3_BYTE;
    b1.expand(1, 1);
    bc = {0_BYTE, 3_BYTE, 1_BYTE, 2_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;
}

TEST(Bytes, Push)
{
    const sp::bytes ob1 = {10_BYTE, 11_BYTE}, ob2 = {20_BYTE, 21_BYTE, 22_BYTE};

    sp::bytes b1(3), b2(4, 3, 10), b3(3), b4(10, 3, 0), bc;

    for (uint i = 0; i < b1.size(); i++)
        b4[i] = b3[i] = b2[i] = b1[i] = (sp::byte)(i);

    b1.push_back(ob1);
    bc = {0_BYTE, 1_BYTE, 2_BYTE, 10_BYTE, 11_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b2.push_back(ob2);
    bc = {0_BYTE, 1_BYTE, 2_BYTE, 20_BYTE, 21_BYTE, 22_BYTE};
    EXPECT_TRUE(b2 == bc) << "should be: " << bc << " is: " << b2;

    b3.push_front(ob2);
    bc = {20_BYTE, 21_BYTE, 22_BYTE, 0_BYTE, 1_BYTE, 2_BYTE};
    EXPECT_TRUE(b3 == bc) << "should be: " << bc << " is: " << b3;

    b4.push_front(ob1);
    bc = {10_BYTE, 11_BYTE, 0_BYTE, 1_BYTE, 2_BYTE};
    EXPECT_TRUE(b4 == bc) << "should be: " << bc << " is: " << b4;

    
    sp::bytes b5;
    b5.push_back({3_BYTE, 4_BYTE});
    b5.push_front({1_BYTE, 2_BYTE});
    bc = {1_BYTE, 2_BYTE, 3_BYTE, 4_BYTE};
    EXPECT_TRUE(b5 == bc) << "should be: " << bc << " is: " << b5;

    sp::bytes b6(1);
    b6.push_back({3_BYTE, 4_BYTE});
    b6.push_front({1_BYTE, 2_BYTE});
    bc = {1_BYTE, 2_BYTE, 0_BYTE, 3_BYTE, 4_BYTE};
    EXPECT_TRUE(b6 == bc) << "should be: " << bc << " is: " << b6;

    sp::bytes b7(2, 1, 2);
    b7.push_back({3_BYTE, 4_BYTE});
    b7.push_front({1_BYTE, 2_BYTE});
    bc = {1_BYTE, 2_BYTE, 0_BYTE, 3_BYTE, 4_BYTE};
    EXPECT_TRUE(b7 == bc) << "should be: " << bc << " is: " << b7;
}

TEST(Bytes, Sub)
{
    sp::bytes b1(10), bc, b;

    for (uint i = 0; i < b1.size(); i++)
        b1[i] = (sp::byte)(i + 10);
    
    //std::cout << b1 << std::endl;

    bc = {10_BYTE, 11_BYTE};
    b = b1.sub(b1.begin(), b1.begin() + 2);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
    b = b1.sub(0, 2);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;

    bc = {17_BYTE, 18_BYTE, 19_BYTE};
    b = b1.sub(b1.end() - 3, b1.end());
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
    b = b1.sub(7, 3);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;

    bc = {17_BYTE};
    b = b1.sub(7, 1);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
}

TEST(Bytes, Shrink)
{
    sp::bytes b1(5), bc;
    b1.set(100_BYTE);

    bc = {100_BYTE, 100_BYTE, 100_BYTE, 100_BYTE, 100_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(2, 0);
    bc = {100_BYTE, 100_BYTE, 100_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(2, 0);
    bc = {0_BYTE, 0_BYTE, 100_BYTE, 100_BYTE, 100_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(0, 2);
    bc = {0_BYTE, 0_BYTE, 100_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(0, 2);
    bc = {0_BYTE, 0_BYTE, 100_BYTE, 0_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.set(1, 2, 200_BYTE);
    bc = {0_BYTE, 200_BYTE, 200_BYTE, 0_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(2, 1);
    bc = {200_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(1, 0);
    b1.shrink(1, 1);
    bc = {200_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(2, 2);
    bc = {0_BYTE, 0_BYTE, 200_BYTE, 0_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(3, 3);

    EXPECT_TRUE(b1.size() == 0);

    b1.expand(1, 5);
    bc = {0_BYTE, 0_BYTE, 0_BYTE, 0_BYTE, 0_BYTE, 0_BYTE};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;
}










TEST(Interface, CircularIterator)
{
    sp::bytes b(10);
    for (int i = 0; i < b.size(); i++)
        b[i] = (sp::bytes::value_type)(i);
    
    sp::detail::buffered_interface::circular_iterator it(b.begin(), b.end(), b.begin());

    for (int i = 0; i < b.size(); i++, ++it)
        EXPECT_EQ((sp::bytes::value_type)(i), *it) << "first +=1 loop: " << i;

    for (int i = 0; i < b.size(); i++, ++it)
        EXPECT_EQ((sp::bytes::value_type)(i), *it) << "second +=1 loop: " << i;

    for (int i = 0; i < b.size(); i += 2, it += 2)
        EXPECT_EQ((sp::bytes::value_type)(i), *it) << "first +=2 loop: " << i;

    for (int i = 0; i < b.size(); i += 2, it += 2)
        EXPECT_EQ((sp::bytes::value_type)(i), *it) << "second +=2 loop: " << i;
    
}

TEST(Interface, UnalteredSequential)
{
    sp::loopback_interface interface(0, 1, 255, 10, 64, 256);

    auto data = [l = 0]() mutable {
        sp::bytes b(++l);
        std::generate(b.begin(), b.end(), [pos = 0]() mutable { return (sp::byte)(++pos); });
        return b;
    };
    auto addr = [&](){return (sp::interface::address_type)2;};

    EXPECT_EQ(test_interface(interface, interface.max_data_size(), data, addr), interface.max_data_size());
}

TEST(Interface, UnalteredRandom)
{
    sp::loopback_interface interface(0, 1, 255, 10, 64, 256);

    auto data = [&](){return random_bytes(1, interface.max_data_size());};
    auto addr = [&](){return random(2, 100);};

    EXPECT_EQ(test_interface(interface, 10000, data, addr), 10000);
}

TEST(Interface, CorruptedSequential)
{
    sp::loopback_interface interface(0, 1, 255, 10, 64, 256, [](sp::byte b){
        if (chance(1)) b |= random_byte();
        return b;
    });

    auto data = [l = 0]() mutable {
        sp::bytes b(++l);
        std::generate(b.begin(), b.end(), [pos = 0]() mutable { return (sp::byte)(++pos); });
        return b;
    };
    auto addr = [&](){return (sp::interface::address_type)2;};

    EXPECT_TRUE(test_interface(interface, interface.max_data_size(), data, addr) > 0);
}

TEST(Interface, CorruptedRandom)
{
    sp::loopback_interface interface(0, 1, 255, 10, 64, 256, [](sp::byte b){
        if (chance(0.5)) b |= random_byte();
        return b;
    });

    auto data = [&](){return random_bytes(1, interface.max_data_size());};
    auto addr = [&](){return random(2, 100);};

    EXPECT_TRUE(test_interface(interface, 10000, data, addr) > 0);
}


TEST(Interface, HeavilyCorruptedRandom)
{
    sp::loopback_interface interface(0, 1, 255, 10, 64, 256, [](sp::byte b){
        if (chance(2)) b |= random_byte();
        return b;
    });

    cout << "this test has a high chance of failing because that level of corruption" << endl; 
    cout << "may be too much for the checksums in use, so feel free to disable it" << endl;

    auto data = [&](){return random_bytes(1, interface.max_data_size());};
    auto addr = [&](){return random(2, 100);};

    EXPECT_TRUE(test_interface(interface, 10000, data, addr) > 0);
}


/* TEST(Fragmentation, Transfer)
{
    //sp::loopback_interface interface(0, 1, 10, 64, 256);
    const sp::bytes b1 = {10_BYTE, 11_BYTE, 12_BYTE, 13_BYTE, 14_BYTE}, b2 = {20_BYTE, 21_BYTE, 22_BYTE}, b3 = {30_BYTE, 31_BYTE}, b4 = {40_BYTE};
    sp::stack::loopback lo(0, 1);
    
    // MODE 1
    sp::headers::fragment_8b8b h(sp::headers::fragment_8b8b::message_types::FRAGMENT, 0, 4, 1, 0, 0);
    sp::transfer p(lo.interface.interface_id(), h);

    sp::bytes bc;
    sp::bytes::size_type i;

    p._assign(2, sp::fragment(2, 3, sp::bytes(b1), lo.interface.interface_id()));
    bc = b1; i = 0;
    for (auto it = p.data_begin(); it != p.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b1 index " << i << ": " << (int)bc[i] << " == " << (int)*it;
    EXPECT_EQ(i, bc.size());

    p._assign(4, sp::fragment(2, 3, sp::bytes(b2), lo.interface.interface_id()));
    bc = b1 + b2; i = 0;
    for (auto it = p.data_begin(); it != p.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b1 + b2 index " << i << ": " << (int)bc[i] << " == " << (int)*it;
    EXPECT_EQ(i, bc.size());

    p._assign(1, sp::fragment(2, 3, sp::bytes(b3), lo.interface.interface_id()));
    bc = b3 + b1 + b2; i = 0;
    for (auto it = p.data_begin(); it != p.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b3 + b1 + b2 index " << i << ": " << (int)bc[i] << " == " << (int)*it;
    EXPECT_EQ(i, bc.size());

    p._assign(3, sp::fragment(2, 3, sp::bytes(b4), lo.interface.interface_id()));
    bc = b3 + b1 + b4 + b2; i = 0;
    for (auto it = p.data_begin(); it != p.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b3 + b1 + b4 + b2 index " << i << ": " << (int)bc[i] << " == " << (int)*it;
    EXPECT_EQ(i, bc.size());


    //MODE2
    //auto t = handler.new_transfer();
    //EXPECT_EQ()
    //TODO
}

TEST(Fragmentation, UnalteredRandom)
{
    sp::stack::loopback lo(0, 1);

    auto data = [&](){return random_bytes(1, lo.interface.max_data_size() * 2);};
    auto addr = [&](){return random(2, 100);};

    EXPECT_EQ(test_handler(lo.interface, lo.fragmentation, 100, data, addr), 100);
}

TEST(Fragmentation, IdOverflow)
{
    sp::stack::loopback lo(0, 1);

    auto data = [&](){return random_bytes(1, lo.interface.max_data_size() * 2);};
    auto addr = [&](){return random(2, 100);};

    while (250 > sp::global_id_factory.new_id(lo.interface.interface_id()));
    cout << "starting with id " << sp::global_id_factory.new_id(lo.interface.interface_id()) << endl;
    EXPECT_EQ(test_handler(lo.interface, lo.fragmentation, 10, data, addr), 10);
}

TEST(Fragmentation, CorruptedRandom)
{
    sp::stack::loopback lo(0, 1, [](sp::byte b){
        if (chance(0.5)) b |= random_byte();
        return b;
    });

    auto data = [&](){return random_bytes(1, lo.interface.max_data_size() * 2);};
    auto addr = [&](){return random(2, 100);};

    EXPECT_EQ(test_handler(lo.interface, lo.fragmentation, 100, data, addr, 25), 100);
}

TEST(Fragmentation, CorruptedRandomLarge)
{
    sp::stack::loopback lo(0, 1, [](sp::byte b){
        if (chance(0.5)) b |= random_byte();
        return b;
    });

    auto data = [&](){return random_bytes(lo.interface.max_data_size() * 5, lo.interface.max_data_size() * 10);};
    auto addr = [&](){return random(2, 100);};

    EXPECT_EQ(test_handler(lo.interface, lo.fragmentation, 10, data, addr, 100), 10);
}


TEST(Ports, PacketConstructor)
{
    const sp::bytes b1 = {10_BYTE, 11_BYTE, 12_BYTE, 13_BYTE, 14_BYTE}, b2 = {20_BYTE, 21_BYTE, 22_BYTE}, b3 = {30_BYTE, 31_BYTE};
    sp::stack::loopback lo(0, 1);
    
    sp::headers::ports_8b h(2, 3);
    sp::transfer t(lo.interface.interface_id());
    t.push_back(b1);
    t.push_front(b2);
    t.push_back(b3);
    t.set_destination(10);
    
    sp::packet p(std::move(t), h);

    sp::bytes bc;
    sp::bytes::size_type i;

    bc = b2 + b1 + b3; i = 0;
    for (auto it = p.data_begin(); it != p.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b2 + b1 + b3 index " << i << ": " << (int)bc[i] << " == " << (int)*it;

    EXPECT_TRUE(bc == p.data_contiguous());
    //EXPECT_EQ(p.get_id(), 2); //TODO
    //EXPECT_EQ(p.get_prev_id(), 1);
    EXPECT_EQ(p.source_port(), 3);
    EXPECT_EQ(p.destination_port(), 2);
    EXPECT_EQ(p.destination(), 10);

    sp::transfer t2(std::move(p.to_transfer()));

    i = 0;
    for (auto it = t2.data_begin(); it != t2.data_end(); ++it, ++i)
        EXPECT_TRUE(bc[i] == *it) << "b2 + b1 + b3 index " << i << ": " << (int)bc[i] << " == " << (int)*it;

    EXPECT_TRUE(bc == t2.data_contiguous());
    //EXPECT_EQ(t2.get_id(), 2);
    //EXPECT_EQ(t2.get_prev_id(), 1);
    EXPECT_EQ(t2.destination(), 10);
} */

/* TEST(Ports, PortsPing)
{
    sp::loopback_interface interface(0, 1, 10, 64, 256);
    sp::fragmentation_handler handler(interface.max_data_size(), 100ms, 10ms, 2);
    sp::ports_handler ports;
    sp::

    handler.bind_to(interface);
    ports.register_interface(interface.interface_id(), handler);

    
} */

