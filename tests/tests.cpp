
#include <libprotoserial/container.hpp>

#include "gtest/gtest.h"

using namespace std;
using namespace sp::literals;




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

    sp::bytes b4 = {1_B, 10_B, 25_B};
    EXPECT_TRUE(b4.size() == 3);
    EXPECT_TRUE(b4[0] == 1_B);
    EXPECT_TRUE(b4[1] == 10_B);
    EXPECT_TRUE(b4[2] == 25_B);

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

    bc = {0_B, 0_B, 0_B, 0_B, 0_B};
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {255_B, 255_B, 255_B, 255_B, 255_B};
    b1.set(255_B);
    b2.set(255_B);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_B, 2_B, 255_B, 255_B, 255_B};
    b1.set(0, 2, 2_B);
    b2.set(0, 2, 2_B);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_B, 2_B, 3_B, 3_B, 3_B};
    b1.set(2, 3, 3_B);
    b2.set(2, 3, 3_B);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    bc = {2_B, 4_B, 4_B, 4_B, 3_B};
    b1.set(1, 3, 4_B);
    b2.set(1, 3, 4_B);
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;

    EXPECT_THROW(b1.set(0, 6, 10_B), sp::out_of_range);
    EXPECT_THROW(b2.set(0, 6, 10_B), sp::out_of_range);
    EXPECT_THROW(b1.set(3, 6, 11_B), sp::out_of_range);
    EXPECT_THROW(b2.set(3, 6, 11_B), sp::out_of_range);

    bc = {10_B, 10_B, 10_B, 11_B, 11_B};
    EXPECT_TRUE(b1 == bc) << bc;
    EXPECT_TRUE(b2 == bc) << bc;
}

TEST(Bytes, Expand)
{
    const sp::bytes bo = {1_B, 2_B, 3_B};
    std::vector<std::unique_ptr<sp::bytes>> vb;
    vb.push_back(std::make_unique<sp::bytes>(3));
    vb.push_back(std::make_unique<sp::bytes>(2, 3, 0));
    vb.push_back(std::make_unique<sp::bytes>(0, 3, 2));
    vb.push_back(std::make_unique<sp::bytes>(2, 3, 2));
    sp::bytes::pointer pb3 = vb.at(3)->_base();

    for (auto& b : vb)
    {
        sp::bytes bc = bo;
        
        std::copy(bo.begin(), bo.end(), b->begin());
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;

        b->expand(1, 0);
        bc = {0_B, 1_B, 2_B, 3_B};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
        b->at(0) = 10_B;

        b->expand(0, 1);
        bc = {10_B, 1_B, 2_B, 3_B, 0_B};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
        b->at(4) = 20_B;

        b->expand(1, 1);
        bc = {0_B, 10_B, 1_B, 2_B, 3_B, 20_B, 0_B};
        EXPECT_TRUE(*b == bc) << "should be: " << bc << " is: " << *b;
    }

    EXPECT_TRUE(pb3 == vb.at(3)->_base()) << "at(3) should not relocate";

    sp::bytes b1, bc;
    b1.expand(1, 0);
    b1[0] = 1_B;
    b1.expand(0, 1);
    b1[1] = 2_B;
    b1.expand(1, 0);
    b1[0] = 3_B;
    b1.expand(1, 1);
    bc = {0_B, 3_B, 1_B, 2_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;
}

TEST(Bytes, Push)
{
    const sp::bytes ob1 = {10_B, 11_B}, ob2 = {20_B, 21_B, 22_B};

    sp::bytes b1(3), b2(4, 3, 10), b3(3), b4(10, 3, 0), bc;

    for (uint i = 0; i < b1.size(); i++)
        b4[i] = b3[i] = b2[i] = b1[i] = (sp::byte)(i);

    b1.push_back(ob1);
    bc = {0_B, 1_B, 2_B, 10_B, 11_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b2.push_back(ob2);
    bc = {0_B, 1_B, 2_B, 20_B, 21_B, 22_B};
    EXPECT_TRUE(b2 == bc) << "should be: " << bc << " is: " << b2;

    b3.push_front(ob2);
    bc = {20_B, 21_B, 22_B, 0_B, 1_B, 2_B};
    EXPECT_TRUE(b3 == bc) << "should be: " << bc << " is: " << b3;

    b4.push_front(ob1);
    bc = {10_B, 11_B, 0_B, 1_B, 2_B};
    EXPECT_TRUE(b4 == bc) << "should be: " << bc << " is: " << b4;

    
    sp::bytes b5;
    b5.push_back({3_B, 4_B});
    b5.push_front({1_B, 2_B});
    bc = {1_B, 2_B, 3_B, 4_B};
    EXPECT_TRUE(b5 == bc) << "should be: " << bc << " is: " << b5;

    sp::bytes b6(1);
    b6.push_back({3_B, 4_B});
    b6.push_front({1_B, 2_B});
    bc = {1_B, 2_B, 0_B, 3_B, 4_B};
    EXPECT_TRUE(b6 == bc) << "should be: " << bc << " is: " << b6;

    sp::bytes b7(2, 1, 2);
    b7.push_back({3_B, 4_B});
    b7.push_front({1_B, 2_B});
    bc = {1_B, 2_B, 0_B, 3_B, 4_B};
    EXPECT_TRUE(b7 == bc) << "should be: " << bc << " is: " << b7;
}

TEST(Bytes, Sub)
{
    sp::bytes b1(10), bc, b;

    for (uint i = 0; i < b1.size(); i++)
        b1[i] = (sp::byte)(i + 10);
    
    //std::cout << b1 << std::endl;

    bc = {10_B, 11_B};
    b = b1.sub(b1.begin(), b1.begin() + 2);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
    b = b1.sub(0, 2);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;

    bc = {17_B, 18_B, 19_B};
    b = b1.sub(b1.end() - 3, b1.end());
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
    b = b1.sub(7, 3);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;

    bc = {17_B};
    b = b1.sub(7, 1);
    EXPECT_TRUE(b == bc) << "should be: " << bc << " is: " << b;
}

TEST(Bytes, Shrink)
{
    sp::bytes b1(5), bc;
    b1.set(100_B);

    bc = {100_B, 100_B, 100_B, 100_B, 100_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(2, 0);
    bc = {100_B, 100_B, 100_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(2, 0);
    bc = {0_B, 0_B, 100_B, 100_B, 100_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(0, 2);
    bc = {0_B, 0_B, 100_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(0, 2);
    bc = {0_B, 0_B, 100_B, 0_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.set(1, 2, 200_B);
    bc = {0_B, 200_B, 200_B, 0_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(2, 1);
    bc = {200_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(1, 0);
    b1.shrink(1, 1);
    bc = {200_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.expand(2, 2);
    bc = {0_B, 0_B, 200_B, 0_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;

    b1.shrink(3, 3);

    EXPECT_TRUE(b1.size() == 0);

    b1.expand(1, 5);
    bc = {0_B, 0_B, 0_B, 0_B, 0_B, 0_B};
    EXPECT_TRUE(b1 == bc) << "should be: " << bc << " is: " << b1;
}



