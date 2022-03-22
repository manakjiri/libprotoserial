
/* #include "libprotoserial/fragmentation.hpp"
#include "tests/helpers/random.hpp"

using namespace sp::literals;
using namespace std;

int main(int argc, char const *argv[])
{
    sp::uart_interface interface("/dev/ttyACM0", B115200, 0, 2, 10, 64, 256);
    sp::fragmentation_handler handler(interface.interface_id(), interface.max_data_size(), 10ms, 100ms, 5);
    handler.bind_to(interface);
    
    int i = 0;
    handler.transfer_receive_event.subscribe([&](sp::transfer f){
        cout << "RX" << i << ": " << f << endl;
    });
    handler.transfer_ack_event.subscribe([](sp::transfer_metadata tm){
        cout << "ACK" << endl;
    });

    for (; i < 5; ++i)
    {
        //sp::bytes data = random_bytes(2, interface.max_data_size());
        sp::bytes data = random_bytes(3);
        
        sp::transfer t(interface.interface_id());
        t.push_back(data);
        t.set_destination(1);
        cout << "TX" << i << ": " << t << endl;
        handler.transmit(std::move(t));
        
        int loops = 15;
        while (loops--)
        {
            interface.main_task();
            handler.main_task();
        }
    }
    

    //cout << "Sent" << endl;

    return 0;
} */

#define SP_BUFFERED_DEBUG

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

    for (; i < 1; ++i)
    {
        sp::bytes data = random_bytes(5, interface.max_data_size());
        //sp::bytes data = random_bytes(3);
        
        auto f = sp::fragment(1, sp::bytes(data));
        cout << "TX" << i << ": " << f << endl;
        interface.write(std::move(f));

        
        int loops = 10;
        while (loops--)
            interface.main_task();
    }
    

    //cout << "Sent" << endl;

    return 0;
}



/* 
900510793
900510793
900510793
 */
