/*
 * This file is a part of the libprotoserial project
 * https://github.com/georges-circuits/libprotoserial
 * 
 * Copyright (C) 2022 Jiří Maňák - All Rights Reserved
 * For contact information visit https://manakjiri.eu/
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/gpl.html>
 */


#ifndef _SP_FRAGMENTATION_TRANSFERHANDLER
#define _SP_FRAGMENTATION_TRANSFERHANDLER

#include "libprotoserial/fragmentation/transfer.hpp"

#include <optional>


namespace sp
{
    namespace detail
    {
        /* extends the functionality of the transfer object in order to handle transmit and receive operations */
        template<typename Header>
        class transfer_handler : public transfer
        {
            public:

            enum class state
            {
                NEW,
                SENT,
                NEXT,
                WAITING,
                RETRY
            };

            /* receive constructor, f is the first fragment of the transfer (maximum fragment size is derived
            from this fragment's size)
            note that this cannot infer the actual size of the final transfer based on this fragment (unless this
            is the last one as well), so a worst-case scenario is assumed (fragments_total * max_fragment_size) 
            and the internal data() container will get resized in the put_fragment function when the last fragment 
            is received */
            transfer_handler(fragment && f, const Header & h) : 
                transfer(transfer_metadata(f.source(), f.destination(), f.interface_id(), f.timestamp_creation(), h.get_id(), 
                h.get_prev_id()), std::move(f.data())), max_fragment_size(0), fragments_total(h.fragments_total()),
                current_fragment(0), sent_at(never()), transfer_state(state::NEW)
            {
                /* reserve space for up to fragments_total fragments. There is no need to regard prealloc_size since this 
                is the receive constructor. fragments_total is always >= 1, so this works for all cases, expand does nothing 
                for arguments (0, 0) */
                max_fragment_size = data().size();
                data().expand(0, (fragments_total - 1) * max_fragment_size);
                /* note that after the receive is done, the transfer's data will get resized using the bytes::shrink function,
                this operation only decreases the size, but does not change capacity, so there is no copy overhead */
            }

            /* transmit constructor, max_fragment_size is the maximum fragment data size excluding the fragmentation header */
            transfer_handler(transfer && t, data_type::size_type max_fragment_size) : 
                transfer(std::move(t)), max_fragment_size(max_fragment_size), fragments_total(0),
                current_fragment(0), sent_at(never()), transfer_state(state::NEW)
            {
                auto size = data().size();
                /* calculate the fragments_total count correctly, ie. assume max = 4, then
                for size = 2 -> total = 1
                for size = 4 -> total = 1 
                but for size = 5 -> total = 2 */
                fragments_total = size / max_fragment_size + (size % max_fragment_size == 0 ? 0 : 1);
            }

            /* returns the fragment's data size, this does not include the Header,
            use only when constructed using the transmit constructor */
            data_type::size_type fragment_size(index_type pos)
            {
                pos -= 1;
                if (pos >= fragments_total)
                    return 0;
                
                auto start = pos * max_fragment_size;
                auto end = std::min(start + max_fragment_size, data().size());
                return end - start;
            }

            /* returns a fragment containing the correct metadata with data copied from pos of the transfer,
            the data does not contain handler's header! it does however have enough prealloc for it plus
            the requested prealloc. Just use fragment.data().push_front(header_bytes) to add the header */
            std::optional<fragment> get_fragment(index_type pos, const prealloc_size & alloc)
            {
                auto data_size = fragment_size(pos);
                if (data_size == 0)
                    return std::nullopt;
                
                /* allocate the container using the prealloc_size class, add additional space for 
                the fragmentation Header */
                bytes data = alloc.create(sizeof(Header), data_size, 0);
                auto start = fragment_data_begin(pos);

                std::copy(start, start + data_size, data.begin());
                return fragment(std::move(get_fragment_metadata()), std::move(data));
            }

            /* this function assumes that this was created using the receive constructor, it only 
            concerns itself with the fragment's data, not its metadata. */
            bool put_fragment(index_type pos, const fragment & f)
            {
                auto expected_max_size = fragment_size(pos);
                if (f.data().size() > expected_max_size)
                    return false;
            
                auto start = fragment_data_begin(pos);
                std::copy(f.data().begin(), f.data().end(), start);

                /* resize the data() so it reflects the actual size now that we have received
                the last fragment, more on that in the receive constructor comment */
                if (pos == fragments_total)
                    data().shrink(0, expected_max_size - f.data().size());

                return true;
            }

            /* maximum fragment data size excluding the fragmentation header,
            max_fragment_size * fragments_total >= data().size() should always hold */
            data_type::size_type max_fragment_size;
            /* max_fragment_size * fragments_total >= data().size() should always hold */
            index_type fragments_total;
            /* timestamp of the last transmit */
            index_type current_fragment;
            /* state of the transmission/reception process */
            clock::time_point sent_at;
            /* index of the fragment to be transmitted, 0 is invalid, ranges from 1 to fragments_total */
            state transfer_state;

            protected:
            
            inline data_type::iterator fragment_data_begin(index_type pos)
            {
                return data().begin() + ((pos - 1) * max_fragment_size);
            }
        };
    }
}


#endif
