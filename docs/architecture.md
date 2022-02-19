# Bytes

basic
- default constructor
- range, begin/end iterator, constructor (copy and move)
- destructor
- copy, move assingment
- begin and end
- size, capacity
- reserve
- operator[], at

extended (implemented from basic)
- 








# Stream

Consists of a consumer object and a producer object. It transports bytes container objects. It is the most bare bones interface aimed at standardizing this consumer-producer relationship so that higher-code can just use these primitives.

The consumer namely implements the write function. There is only one slot for a data container within the stream, consumer can be written to only when this slot is empty, otherwise an exception is raised

The producer has registerable callback slot to which another consumer can be bound.
 



# Protocol

## Ports

The ports protocol allows for condesing multiple streams into one by prepending the data with a single byte which acts as an address field.




