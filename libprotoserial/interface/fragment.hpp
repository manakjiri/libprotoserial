

#ifndef _SP_INTERFACE_FRAGMENT
#define _SP_INTERFACE_FRAGMENT

#include "libprotoserial/container.hpp"
#include "libprotoserial/clock.hpp"

namespace sp
{
    class interface;

    class fragment_metadata
    {
        public:
        using address_type = uint;

        fragment_metadata(address_type src, address_type dst, const interface *i, clock::time_point timestamp_creation):
            _timestamp_creation(timestamp_creation), _interface(i), _source(src), _destination(dst) {}

        fragment_metadata(const fragment_metadata &) = default;
        fragment_metadata(fragment_metadata &&) = default;
        fragment_metadata & operator=(const fragment_metadata &) = default;
        fragment_metadata & operator=(fragment_metadata &&) = default;

        constexpr clock::time_point timestamp_creation() const {return _timestamp_creation;}
        constexpr const interface* get_interface() const noexcept {return _interface;}
        constexpr address_type source() const noexcept {return _source;}
        constexpr address_type destination() const noexcept {return _destination;}

        void set_destination(address_type dst) {_destination = dst;}

        protected:
        clock::time_point _timestamp_creation;
        const interface *_interface;
        address_type _source, _destination;
    };

    /* interface fragment representation */
    class fragment : public fragment_metadata
    {
        public:

        typedef bytes   data_type;

        fragment(address_type src, address_type dst, data_type && d, const interface *i) :
            fragment_metadata(src, dst, i, clock::now()), _data(std::move(d)) {}

        /* this object can be passed to the interface::write() function */
        fragment(address_type dst, data_type && d) :
            fragment((address_type)0, dst, std::move(d), nullptr) {}

        fragment():
            fragment(0, data_type()) {}

        fragment(const fragment &) = default;
        fragment(fragment &&) = default;
        fragment & operator=(const fragment &) = default;
        fragment & operator=(fragment &&) = default;
        
        constexpr const data_type& data() const noexcept {return _data;}
        constexpr data_type& data() noexcept {return _data;}
        constexpr void _complete(address_type src, const interface *i) {_source = src; _interface = i;}
        
        bool carries_information() const {return _data && _destination;}
        explicit operator bool() const {return carries_information();}

        protected:
        data_type _data;
    };
}


#endif

