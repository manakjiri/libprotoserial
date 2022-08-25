# Interface

abstracts away the physical interface, for that it needs to handle - encoding, error detection and correction

interfaces are inherently interrupt driven, we need to either ensure that callbacks don't take a long time or we resort to buffering, which seems like the safest and most versatile bet. But now we need to determine when the actual processing takes place.

it seems that the right balance would be: decode, check, correct and determine whether the fragment belongs to us within the interrupt routine - that way only the relevant stuff gets put into the processing queue.

in terms of structure it would be nice for the interface to only handle the the bits and not have to deal with the more abstract things, like addressing -> let's introduce another component (address class) that deals with that in order to maintain modularity.

USB
- one interrupt callback for receive, fragments are already pieced together
- somewhat error checked
- 64B fragment size limit

UART
- need to receive byte-by-byte in an interrupt (we could make it more efficient if we could constraint the fragment size to something like 8B multiples so that we benefit from the hardware)
- don't know the size in advance, it should be constrained though

it would be nice to somehow ignore fragments that are not meant for us, which can be done if the address class exposes how long the address should be and then vets tha fragment. But we cannot necessarily stop listening as soon as we figure this out, because we need to catch the end of the fragment + it may be useful to have a callback with fragments that were not meant to us, but someone may be interested for debugging purposes for example.

so interface must already be concerned with addresses as well as with all the lower layer stuff. However it should not be handling fragment  fragmentation, I'd leave that to the thing that processes the RX queue.

which leads us to what the interface class should expose
- an RX event
- a bad RX event
- a TX complete event

interface has an RX and a TX queue, it will throw an exception? when the TX fragment is over the maximum length. Events are fired from the main interface task, not from the interrupts, because event callbacks could take a long time to return. Queues isolate the interrupts from the main task. 

We can build the fragment fragmentation logic on top of the interface, this logic should have its own internal buffer for fragments received from events, because once the event firess, the fragment is forgotten on the interface side to avoid the need for direct access to the interface's RX queue.

the TX queue should be size limited and an interface should be exposed for checking whether we can push to the queue without being blocked by the function call because the queue is too large. 

so it should also expose
- write(fragment[, callback?]) a potentially blocking call
- writable() or something similar

Let's think again... Some interfaces are, as we said, easier to handle, like USB. Can these simple ones be a subset of the harder ones, like UART?

For something like a uart it may be best to have some contiguous static buffer that we use as a ring buffer - here the isr is really simple, it just puts each byte it receives into the buffer and increments an index and handles the index wrap. 

It may be best to introduce a start sequence into the fragments, something like 0x5555 or whatever, which we use to lookup the start of the fragment in this large array sort of efficiently. How to handle the wrap?

## estimating the interface load

- 100% load will occur when the interface spends all of its time sending/receiving bytes
- for receive we can observe the ring buffer - every time we enter the do_receive function we can look at the amount of bytes the buffer accumulated since the last call of the function
- for transmit we can simply count bytes we are about to transmit

how do we express this load?
- something per second is an extremely slowly updating metric
    - what about /100ms?, /10ms slots?
- rolling average should be quite representative
- raw bytes received/transmitted should definitely be available
    - perhaps a good start, you can figure out a lot from this
    - lets say 1MB/s for a year is 1M*365*24*3600 = 31536*10^9 = 3.1536*10^13, uint32 is ~4.29*10^9, uint64 is ~1,84e19 - that should do it
