
#include <libprotoserial/byte.hpp>

namespace sp
{
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
            INIT, STATIC, HEAP
        };

        /* blank of INIT type */
        bytes();
        /* HEAP type is assumed */
        bytes(uint length);
        /* just initialization with the given arguments, no copying */
        bytes(byte* data, uint length, alloc_t type);
        /* copy */
        bytes(const bytes & other);
        bytes & operator= (const bytes & other); 
        /* move */
        bytes(bytes && other);
        bytes & operator= (bytes && other);

        /* destructor calls the clear */
        ~bytes();
        
        //bytes(const std::string & from) {}        // convert from string 
        //operator std::string() const {return };
                
        
        /* returns the current configured size, if overprovisioning is not used than size == capacity */
        uint size() const;
        /* returns the number of actually allocated bytes */
        uint capacity() const;
        /* 
        byte* data() const;
        alloc_t type() const;
        
        const byte & at(uint i) const;
        byte & at(uint i);
        const byte & operator[] (uint i) const;
        byte & operator[] (uint i);
        
        /* converts the container to HEAP type if necessary and expands it by the requested amount
        from each side with value: [front B][size B][back B], front or back can be 0 */
        void expand(uint front, uint back, byte value);
        /* just as above with value = 0 */
        void expand(uint front, uint back);
        /* expand the container by other.size() bytes and copy other's contents into that space */
        void prepend(const bytes & other);
        void append(const bytes & other);
        
        /* set all bytes to value */
        void set(byte value);
        /* safe to call multiple times, frees the resources for the HEAP type and sets up the
        container as if it was just initialized using the default constructor */
        void clear();
        /* used internally for move, do not use otherwise */
        void _init();

        /* converts the container to HEAP allocated container */
        void to_heap();

        
        protected:
        byte *_data;
        uint _length, _capacity, _offset;
        alloc_t _type;

        void range_check(uint i) const;
        void copy_from(const byte* data, uint length);
        void copy_to(byte* data, uint length) const;
        void alloc(uint length);
    };
}

bool operator==(const sp::bytes & lhs, const sp::bytes rhs);
bool operator!=(const sp::bytes & lhs, const sp::bytes rhs);


//sp::dynamic_bytes & operator+ (const sp::dynamic_bytes & lhs, const sp::dynamic_bytes rhs);


