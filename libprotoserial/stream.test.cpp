#include <libprotoserial/stream.hpp>

#include <string>
#include <memory>
#include <iostream>

using namespace std;



int main(int argc, char const *argv[])
{
    static char some_data[20] = "hello world";
    sp::dynamic_bytes d((sp::byte*)some_data, sizeof(some_data));

    auto print = make_shared<sp::cout_stream>(), print2 = make_shared<sp::cout_stream>();
    auto loop = make_shared<sp::loopback_stream>(), loop2 = make_shared<sp::loopback_stream>();
    
    loop->bind(print);

    loop2->bind(print2);
    
    loop2->write(make_unique<sp::bytes>(d));

    return 0;
}



