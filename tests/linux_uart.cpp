
#include "libprotoserial/interface.hpp"

using namespace sp::literals;
using namespace std;

int main(int argc, char const *argv[])
{
    sp::uart_interface interface("/dev/ttyACM0", B115200, 0, 2, 10, 64, 256);
    
    sp::bytes data = {0x11_B, 0x22_B, 0x42_B};
    
    interface.receive_event.subscribe([](sp::fragment f){
        cout << "RX: " << f << endl;
    });

    for (int i = 0; i < 5; ++i)
    {
        interface.write(sp::fragment(1, sp::bytes(data)));
        interface.main_task();
        interface.main_task();
        interface.main_task();
    }
    

    //cout << "Sent" << endl;

    return 0;
}


