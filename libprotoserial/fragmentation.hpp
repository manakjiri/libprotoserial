/* 
 * > We can build the packet fragmentation logic on top of the interface, 
 * > this logic should have its own internal buffer for packets received
 * > from events, because once the event firess, the packet is forgotten on 
 * > the interface side to avoid the need for direct access to the interface's 
 * > RX queue.
 * 
 * 
 * 
 * 
 */




#ifndef _SP_FRAGMENTATION
#define _SP_FRAGMENTATION

#include "libprotoserial/interface.hpp"

namespace sp
{
    class fragmentation_handler
    {
        public:
        
        fragmentation_handler(interface * i):
            _interface(i)
        {
            _rx_subscription = _interface->packed_rxed_event.subscribe(rx_callback, this);
            
        }

        private:

        void rx_callback(interface::packet p)
        {

        }

        interface * _interface;
        subscription _rx_subscription;
    };

}








#endif
