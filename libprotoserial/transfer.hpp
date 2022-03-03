/* 
 * The point of the packet is to provide a useful abstraction for the 
 * fragmentation_handler. It holds multiple interface::packets and provides
 * an iterator to access that data without copying.
 * 
 * For this it needs to
 * - store packets and keep their order, allow inserting packets to requested
 *   positions (like a linked list)
 * 
 * 
 * but it also needs to be usable when constructing the transfer, we need to allow
 * appending and prepending of data, not packets, and make it as efficient
 * as possible. To achieve that, I'd allow push_front/back into the internal vector
 * and then fragment that inside the fragmentation handler, since it needs
 * to add its headers anyway.
 * 
 * MODIFIERS
 * needed by fragmentation_handler
 * - assign(packet)
 * - to_fragmented(max_packet_size) should it return a new transfer?
 * 
 * potentially needed by constructee
 * - push_back(data)
 * - push_front(data)
 * 
 * 
 * 
 * 
 */




#ifndef _SP_TRANSFER
#define _SP_TRANSFER

#include "libprotoserial/interface.hpp"

#include <vector>
#include <algorithm>


namespace sp
{
    //class fragmentation_handler;
    

}





#endif

