
/* 
a simple example demonstrating the lowest communication layer - the interface.

build and run using the run.sh script
 */


#include <libprotoserial/interface.hpp>
#include <libprotoserial/testing/simulation.hpp>
#include <libprotoserial/testing/random.hpp>

#include <queue>
#include <chrono>

using namespace std::chrono_literals;
using namespace sp::literals;


int main(int argc, char const *argv[])
{
    /* setup the loopback interface */
    sp::loopback_interface interface (
        0,   /* 0'th interface instance (if we had multiple loopback interfaces, we'd want to distinguish them somehow) */
        1,   /* address 1 (the interface only receives fragments where f.destination() == 1, more on that later) */
        255, /* broadcast address 255 (the interface also receives any fragment where f.destination() == 255) */
        10,  /* maximum transmit queue size is 10 (the interface will store up to 10 fragments pending transmit) */
        64,  /* maximum fragment size is 64 bytes - do not transmit more than about 50 bytes of data */
        256  /* the interface has a circular receive buffer of 256 bytes */
    );

    /* the address thing - loopback interface is weird in the way it handles addresses and is only really useful for
    testing interfaces alone, because its behaviour confuses the upper layers. */

    /* a queue of transmitted fragments so we can cross-check them whenever we receive anything */
    std::queue<sp::fragment> transmitted;
    /* a mutex to prevent a data race between main_task() and transmit() */
    std::mutex interface_mutex;
    using lock_guard = std::lock_guard<std::mutex>;


    /* a convenience lambda function so that we do not have to repeat those two lines everywhere */
    auto transmit = [&](sp::fragment f){
        std::cout << "transmitting: " << f << std::endl;
        const lock_guard lock(interface_mutex);
        /* store a copy of the fragment in the queue */
        transmitted.push(f);
        /* transmit the original fragment using the loopback interface */
        interface.transmit(std::move(f));
    };

    /* subscribe to the receive event of the interface, this lambda will get called
    every time the interface receives a new fragment */
    interface.receive_event.subscribe([&](sp::fragment received){
        std::cout << "received:     " << received;
        /* the interface does not shuffle the fragments, so the one we have in the 
        "transmitted" queue should match the one we've just received */
        auto orig = transmitted.front();
        transmitted.pop();
        /* check that the addresses and data actually match the original, the else
        clause should never be reached, unless something breaks of course */
        if (received.source() == orig.destination() && received.data() == orig.data())
            std::cout << " matches original" << std::endl;
        else
            std::cout << " does not match: " << orig << std::endl;
    });

    /* launch a thread that calls the main_task() function of the interface in a loop 
    we could do this every time we transmit some data, but this brings us closer
    to a real world scenario
    the main_task is where the interface actually processes the received data, 
    so it is continuously checking for newly received bytes */
    sim::loop_thread main_thread([&](){
        const lock_guard lock(interface_mutex);
        interface.main_task();
    });

    /* the setup is finished let's transmit some data */

    /* transmit "hello world" to address 3 */
    std::cout << "hello world:" << std::endl;
    transmit(sp::fragment(3, sp::bytes("hello world")));
    
    /* wait for a tiny bit so we get a nice print */
    std::this_thread::sleep_for(10ms);

    std::cout << "more fragments at once:" << std::endl;
    /* transmit 1, 2, 3 to 42 */
    transmit(sp::fragment(42, {1_BYTE, 2_BYTE, 3_BYTE}));
    /* transmit 5 more random bytes to 42 */
    transmit(sp::fragment(42, random_bytes(5)));


    /* block until we have received everything */
    while(!transmitted.empty());

    return 0;
}


