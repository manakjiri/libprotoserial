
#include "libprotoserial/libconfig.hpp"
#include "libprotoserial/data/container.hpp"

namespace sp
{
    using object_id_type = uint;
    
    /* this class is suitable as a base class, its purpose is to provide trackability of objects 
    with minimum overhead. It is mainly used to track fragments, transfers and packets. */
    struct sp_object
    {
        sp_object() : sp_object(_new_id()) {}

        sp_object(const sp_object &) : sp_object() {}
        sp_object(sp_object && obj) : sp_object(obj.object_id()) {}
        sp_object & operator=(const sp_object &) {_id = _new_id(); return *this;}
        sp_object & operator=(sp_object && obj) {_id = obj.object_id(); return *this;}

        /* this ID is tracked globally and should be unique on a modest
        time scale. All ID values are valid. Move preserves the ID but a Copy
        will generate a new ID for the copy of the original object */
        const object_id_type object_id() const noexcept {return _id;}

        private:

        sp_object(object_id_type id) : _id(id) {}
        
        object_id_type _new_id() 
        {
            static object_id_type _id_count = 0;
            return ++_id_count;
        }

        object_id_type _id;
    };
}


