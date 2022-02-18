
#include <libprotoserial/container.hpp>

#include "gtest/gtest.h"

using namespace std;
using namespace sp::byte_literal;




TEST(Container, Constructor) 
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

TEST(Container, Access)
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

TEST(Container, Copy)
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

TEST(Container, Move)
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

TEST(Container, Set)
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

TEST(Container, Expand)
{
    
}


/* 

int main(int argc, char const *argv[])
{
    //sp::bytes b2 = {1, 2, 3, 4, 5};

    sp::bytes b1 (2, 5, 1);
    b1.set((byte)1);

    b1[0] = (byte)10;
    b1[1] = (byte)((int)b1[0] + 1);

    cout << b1 << endl;

    b1.expand(1, 1);

    cout << b1 << endl;


    sp::bytes b2 ("12345");
    sp::bytes b3 = b2;

    cout << b3 << ' ' << b2 << endl;

    cout << (b2.data() == b3.data()) << ' ' << (b2 == b3) << endl;

    b3 = move(b2);
    
    cout << b3 << ' ' << b2 << endl;

    b2 = move(b3);

    cout << b3 << ' ' << b2 << endl;


    return 0; 
}
 */


