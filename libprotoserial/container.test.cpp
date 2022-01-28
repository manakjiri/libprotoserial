
#include <libprotoserial/container.hpp>

#include <iostream>
#include <iomanip>

using namespace std;


int main(int argc, char const *argv[])
{
    static char some_data[20] = "hello world";
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


    sp::static_bytes data4 = data;

    return 0;
}



