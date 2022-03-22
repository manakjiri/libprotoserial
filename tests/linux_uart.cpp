
#include "libprotoserial/interface.hpp"
#include "tests/helpers/random.hpp"

using namespace sp::literals;
using namespace std;

int main(int argc, char const *argv[])
{
    sp::uart_interface interface("/dev/ttyACM0", B115200, 0, 2, 10, 64, 256);
    
    int i = 0;
    interface.receive_event.subscribe([&](sp::fragment f){
        cout << "RX" << i << ": " << f << endl;
    });

    for (; i < 3; ++i)
    {
        sp::bytes data = random_bytes(5, interface.max_data_size());
        //sp::bytes data = random_bytes(3);
        
        auto f = sp::fragment(1, sp::bytes(data));
        cout << "TX" << i << ": " << f << endl;
        interface.write(std::move(f));

        
        int loops = 15;
        while (loops--)
            interface.main_task();
    }
    

    //cout << "Sent" << endl;

    return 0;
}


