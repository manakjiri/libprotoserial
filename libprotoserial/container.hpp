
#include <libprotoserial/byte.hpp>

#include <initializer_list>
#include <string>

namespace sp
{
    using uint = unsigned int;

/*     class bytes 
    {
        public:
        bytes();
        //bytes(const bytes & other);                     // copy
        //bytes(bytes && other);                          // move
        ~bytes() {};
        
        //bytes(const std::string & from) {}        // convert from string 
        //operator std::string() const {return };
        
        //bytes & operator= (bytes other);                // copy & swap
        const byte & operator[] (uint i) const;
        byte & operator[] (uint i);
        
        
        uint size() const;
        byte* data() const;
        byte & at(uint i);
        const byte & at(uint i) const;
        void set(byte value);
        virtual void clear();

        protected:
        byte *_data;
        uint _length;

        void range_check(uint i) const;
        void copy_from(const byte* data, uint length);
        void copy_to(byte* data, uint length) const;
    };


    class static_bytes : public bytes
    {
        public:
        //static_bytes();
        static_bytes(byte* data, uint length);
        //static_bytes(const bytes & other);

    };


    class dynamic_bytes : public bytes
    {
        public:
        dynamic_bytes(uint length);
        dynamic_bytes(const byte* data, uint length);
        dynamic_bytes(const bytes & other);
        dynamic_bytes(const dynamic_bytes & other);
        dynamic_bytes(dynamic_bytes && other);
        ~dynamic_bytes();

        dynamic_bytes & operator= (const bytes & other); 
        dynamic_bytes & operator= (dynamic_bytes && other);

        void prepend(const bytes & other);
        void append(const bytes & other);
        void clear();

        protected:
        void alloc(uint length);
    }; */

    class bytes 
    {
        public:

        enum alloc_t
        {
            INIT, 
            /* the data is stored and managed internally in a heap-allocated space */
            HEAP,
            /* the container just acts as a reference to some existing data, it is in read-only mode, 
            any attempt at modifing the container will result in the invocation of the to_heap() function 
            and all modifications will than be applied to the internally managed data. */
            HANDLE,
        };

        /* blank of INIT type */
        bytes();
        /* HEAP type is assumed */
        bytes(uint length);
        /* overallocation, HEAP type is assumed, capacity will be equal to front + length + back */
        bytes(uint front, uint length, uint back);
        /* use this if you want to
        1) wrap existing raw array of bytes by this class - use the HEAP type
        2) create a read-only handle of some existing data - use the HANDLE type */
        bytes(byte* data, uint length, alloc_t type);

        //bytes(std::initializer_list<byte> values);
        //operator std::string() const {return };
        bytes(const std::string & from);
        
        /* copy - only the currently exposed data gets coppied, overallocation is not used */
        bytes(const bytes & other);
        bytes & operator= (const bytes & other); 
        /* move */
        bytes(bytes && other);
        bytes & operator= (bytes && other);

        /* destructor calls the clear() */
        ~bytes();
        
        
        /* returns the current data size, if overallocation is not used than size == capacity */
        uint size() const;
        /* returns pointer to data */
        byte* data() const;
        
        const byte & at(uint i) const;
        byte & at(uint i);
        const byte & operator[] (uint i) const;
        byte & operator[] (uint i);
        
        /* converts the container to HEAP type if necessary and expands it by the requested amount
        [front B][size B][back B], front or back can be 0 */
        void expand(uint front, uint back);
        /* expand the container by other.size() bytes and copy other's contents into that space */
        void prepend(const bytes & other);
        void append(const bytes & other);
        
        /* set all bytes to value */
        void set(byte value);
        void set(uint start, uint length, byte value);
        /* safe to call multiple times, frees the resources for the HEAP type and sets up the
        container as if it was just initialized using the default constructor */
        void clear();
        
        /* converts the container to HEAP allocated container */
        void to_heap();
        /* used internally for move, do not use otherwise */
        void _init();
        /* returns the number of actually allocated bytes */
        uint get_capacity() const;
        uint get_offset() const;
        alloc_t get_type() const;
        /* returns pointer to the beggining of the data */
        byte* get_base() const;


        
        protected:
        byte *_data;
        uint _length, _capacity, _offset;
        alloc_t _type;

        void range_check(uint i) const;
        void copy_from(const byte* data, uint length);
        void copy_to(byte* data, uint length) const;
        /* replaces the _data with a newly allocated and initialized array of length, sets the type to
        HEAP, does not change the _capacity nor the _length! */
        void alloc(uint length);
    };
}

bool operator==(const sp::bytes & lhs, const sp::bytes rhs);
bool operator!=(const sp::bytes & lhs, const sp::bytes rhs);


//sp::dynamic_bytes & operator+ (const sp::dynamic_bytes & lhs, const sp::dynamic_bytes rhs);


