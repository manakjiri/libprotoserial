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
        /* extends the functionality of the transfer object in order to handle 
        transmit and receive operations in fragmentation_handler */
        template<typename Header>
        class transfer_handler : public transfer
        {
            public:
            using header_type = Header;

            enum class state
            {
                NEW,
                SENT,
                NEXT,
                WAITING,
                RETRY
            };

            enum class purpose
            {
                OUTGOING,
                INCOMING
            };

            /* receive constructor, f is the first fragment of the transfer (maximum fragment size is derived
            from this fragment's size)
            note that this cannot infer the actual size of the final transfer based on this fragment (unless this
            is the last one as well), so a worst-case scenario is assumed (fragments_total * max_fragment_size) 
            and the internal data() container will get resized in the put_fragment function when the last fragment 
            is received */
            transfer_handler(fragment f, const Header & h) : 
                transfer(transfer_metadata(f, h.get_id(), h.get_prev_id()), std::move(f.data())), max_fragment_size(0), 
                fragments_total(h.fragments_total()), current_fragment(0), last_tx_time(never()), last_rx_time(never()), 
                transfer_state(state::NEXT), transfer_purpose(purpose::INCOMING)
            {
                /* reserve space for up to fragments_total fragments. There is no need to regard prealloc_size since this 
                is the receive constructor. fragments_total is always >= 1, so this works for all cases, expand does nothing 
                for arguments (0, 0) */
                max_fragment_size = data().size();
                data().expand(0, (fragments_total - 1) * max_fragment_size);
                /* note that after the receive is done, the transfer's data will get resized using the bytes::shrink function,
                this operation only decreases the size, but does not change capacity, so there is no copy overhead */
                
                fragment_status.reserve(fragments_total);
                std::fill(fragment_status.begin(), fragment_status.end(), 0);
            }

            /* transmit constructor, max_fragment_size is the maximum fragment data size excluding the fragmentation header */
            transfer_handler(transfer t, data_type::size_type max_fragment_data_size) : 
                transfer(std::move(t)), max_fragment_size(max_fragment_data_size), fragments_total(0),
                current_fragment(0), last_tx_time(never()), last_rx_time(never()), 
                transfer_state(state::NEW), transfer_purpose(purpose::OUTGOING)
            {
                auto size = data().size();
                /* calculate the fragments_total count correctly, ie. assume max = 4, then
                for size = 2 -> total = 1
                for size = 4 -> total = 1 
                but for size = 5 -> total = 2 */
                fragments_total = size / max_fragment_size + (size % max_fragment_size == 0 ? 0 : 1);

                fragment_status.reserve(fragments_total);
                std::fill(fragment_status.begin(), fragment_status.end(), 0);
            }

            /* returns the fragment's data size, this does not include the Header */
            data_type::size_type fragment_size(index_type pos)
            {
                if (pos > fragments_total)
                    return 0;
                
                auto start = (pos - 1) * max_fragment_size;
                auto end = std::min(start + max_fragment_size, data().size());
                return end - start;
            }

            /* returns a fragment containing the correct metadata with data copied from pos of the transfer,
            the data does not contain handler's header! it does however have enough prealloc for it plus
            the requested prealloc. Just use fragment.data().push_front(header_bytes) to add the header */
            std::optional<fragment> get_fragment(index_type pos, const prealloc_size & alloc)
            {
                if (!is_outgoing())
                    return std::nullopt;

                auto data_size = fragment_size(pos);
                if (data_size == 0)
                    return std::nullopt;

                /* we are getting a fragment which is to be transmitted, increments its counter */
                increment_fragment_status(pos);
                
                /* allocate the container using the prealloc_size class, add additional space for 
                the fragmentation Header */
                auto data = alloc.create(sizeof(Header), data_size, 0);
                auto start = fragment_data_begin(pos);

                std::copy(start, start + data_size, data.begin());
                return fragment(std::move(get_fragment_metadata()), std::move(data));
            }

            /* this function assumes that this was created using the receive constructor, it only 
            concerns itself with the fragment's data, not its metadata. */
            bool put_fragment(index_type pos, const fragment & f)
            {
                if (!is_incoming())
                    return false;

                auto expected_max_size = fragment_size(pos);
                if (expected_max_size == 0 || f.data().size() > expected_max_size)
                    return false;
            
                auto start = fragment_data_begin(pos);
                std::copy(f.data().begin(), f.data().end(), start);
                
                /* we have inserted fragment at pos, increment its counter */
                increment_fragment_status(pos);

                /* resize the data() so it reflects the actual size now that we have received
                the last fragment, more on that in the receive constructor comment */
                if (pos == fragments_total)
                    data().shrink(0, expected_max_size - f.data().size());

                return true;
            }

            bool set_retransmit_request(index_type pos)
            {
                /* the retransmit request can only come after we have transmitted the final
                fragment of the transfer. Something is wrong if it comes sooner */
                if (transfer_state != state::SENT || current_fragment != fragments_total)
                    return false;

                if (pos > fragments_total)
                    return false;

                /* set the internal state according to docs */
                transfer_state = state::RETRY;
                current_fragment = pos;
                
                return true;
            }

            bool is_incoming() const
            {
                return transfer_purpose == purpose::INCOMING;
            }

            bool is_outgoing() const
            {
                return transfer_purpose == purpose::OUTGOING;
            }

            /* check if the fragment f should be inserted into this transfer
            this is used to find an incoming transfer (receive constructor) to which a given 
            fragment belongs to */
            bool is_part_of(const fragment & f, const header_type & h) const
            {
                return source() == f.source() && interface_id() == f.interface_id() && 
                    get_id() == h.get_id() && fragments_total == h.fragments_total();
            }

            /* check if the fragment f is requesting certain fragment be retransmitted
            this is used to find an outgoing transfer (transmit constructor) */
            bool is_request_of(const fragment & f, const header_type & h) const
            {
                return destination() == f.source() && interface_id() == f.interface_id() &&
                    get_id() == h.get_id() && fragments_total == h.fragments_total() && 
                    fragments_total >= h.fragment();
            }

            /* check if the fragment f is an ACK fragment of this transfer
            this is used to find an outgoing transfer (transmit constructor) */
            bool is_ack_of(const fragment & f, const header_type & h) const
            {
                /* the conditions are identical */
                return is_request_of(f, h);
            }

            bool is_current_first() const
            {
                return current_fragment == 1;
            }

            bool is_current_last() const
            {
                return current_fragment == fragments_total;
            }

            bool is_current_bordering() const
            {
                return is_current_first() || is_current_last();
            }

            bool is_current_main() const
            {
                return current_fragment > 1 && current_fragment < fragments_total;
            }

            /* this vector holds some status information about the fragments this transfer is made out of
            - INCOMING: counts how many times a certain fragment was received
            - OUTGOING: counts how many times a certain fragment was transmitted */
            std::vector<uint> fragment_status;
            /* timestamp of the last transmit/receive */
            clock::time_point last_tx_time, last_rx_time;
            /* maximum fragment data size excluding the fragmentation header,
            max_fragment_size * fragments_total >= data().size() should always hold */
            data_type::size_type max_fragment_size;
            /* max_fragment_size * fragments_total >= data().size() should always hold */
            index_type fragments_total;
            /* index of the fragment to be transmitted, 0 is invalid, ranges from 1 to fragments_total */
            index_type current_fragment;
            /* state of the transmission/reception process */
            state transfer_state;
            purpose transfer_purpose;

            protected:
            
            inline data_type::iterator fragment_data_begin(index_type pos)
            {
                return data().begin() + ((pos - 1) * max_fragment_size);
            }
            inline void increment_fragment_status(index_type pos)
            {
                fragment_status.at(pos - 1) += 1;
            }
        };
    }
}


#endif
