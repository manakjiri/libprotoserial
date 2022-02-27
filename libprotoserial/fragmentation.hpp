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
#include "libprotoserial/packet.hpp"
#include "libprotoserial/interface/headers.hpp"

namespace sp
{
    class fragmentation_handler
    {
        public:

        using header = headers::fragment_header_8b16b;
        
        fragmentation_handler()
        {
        }


        void receive_callback(interface::packet p)
        {

        }


        subject<interface::packet> transmit_event;        

        private:

    };

}








#endif
