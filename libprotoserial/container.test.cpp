
#include <libprotoserial/container.hpp>

#include <iostream>
#include <iomanip>

using namespace std;


ostream& operator<< (ostream& os, const sp::bytes& obj) 
{
    os << "[ ";
    for (uint i = 0; i < obj.size(); i++)
        os << (int)obj[i] << ' ';
    return os << ']';
}


int main(int argc, char const *argv[])
{
    /* static char some_data[20] = "hello world";
    size_t some_data_len = sizeof(some_data);

    cout << some_data << " " << to_string(some_data_len) << endl;
    
    sp::static_bytes data((sp::byte*)some_data, some_data_len);

    cout << data.data() << " " << to_string(data.size()) << endl;

    data[0] = 'H';

    cout << data.data() << " " << to_string(data.size()) << endl;

    sp::dynamic_bytes data2 = data;

    data2[1] = 'E';

    cout << data.data() << " " << to_string(data.size()) << endl;
    cout << data2.data() << " " << to_string(data2.size()) << endl;


    sp::dynamic_bytes data3 = data2;

    data3[2] = 'L';
    cout << data3.data() << " " << to_string(data3.size()) << endl;

    data3.clear();


    sp::static_bytes data4 = data; */


    sp::bytes b(2, 5, 1);
    b.set(1);

    b[0] = 10;
    b[1] = b[0] + 1;

    cout << b << endl;

    b.expand(2, 1);

    cout << b << endl;

    return 0;
}



