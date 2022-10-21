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

#include <optional>

#include "libprotoserial/fragmentation/transfer.hpp"

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
            using message_types = typename header_type::message_types;

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
                transfer(transfer_metadata(f, h.get_id(), h.get_prev_id()), std::move(f.data())), last_tx_time(never()), 
                last_rx_time(clock::now()), max_fragment_size(0), fragments_total(h.fragments_total()), current_fragment(0),
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
                transfer(std::move(t)), last_tx_time(never()), last_rx_time(never()), max_fragment_size(max_fragment_data_size), 
                fragments_total(0), current_fragment(0), transfer_state(state::NEW), transfer_purpose(purpose::OUTGOING)
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

            /* returns a fragment containing the correct metadata with data copied from pos of the transfer
            and the templated fragmentation Header */
            std::optional<fragment> get_fragment(index_type pos, const prealloc_size & alloc)
            {
                if (!is_outgoing())
                    return std::nullopt;

                auto data_size = fragment_size(pos);
                if (data_size == 0)
                    return std::nullopt;

                if (!prepare_for_transmit())
                    return std::nullopt;

                /* we are getting a fragment which is to be transmitted, increments its counter */
                increment_fragment_status(pos);
                
                /* allocate the container using the prealloc_size class, add additional space for 
                the fragmentation Header */
                auto data = alloc.create(sizeof(Header), data_size, 0);
                auto start = fragment_data_begin(pos);
                
                /* copy the data from the internal data buffer */
                std::copy(start, start + data_size, data.begin());

                /* copy the header */
                Header h = create_header(message_types::FRAGMENT, current_fragment);
                data.push_front(to_bytes(h));

                fragment ret(std::move(get_fragment_metadata()), std::move(data));
                transmitted_fragment_id = ret.object_id();
                
                return ret;
            }

            /* returns the current fragment containing the correct metadata with data copied 
            from the transfer and the templated fragmentation Header */
            inline std::optional<fragment> get_current_fragment(const prealloc_size & alloc)
            {
                return get_fragment(current_fragment, alloc);
            }

            std::optional<fragment> get_ack_fragment(const prealloc_size & alloc)
            {
                if (!is_incoming() || transfer_state != state::NEXT)
                    return std::nullopt;

                if (current_fragment == 0 || current_fragment > fragments_total)
                    return std::nullopt;

                auto data = alloc.create(sizeof(Header), 0, 0);

                Header h = create_header(message_types::ACK, current_fragment);
                data.push_front(to_bytes(h));
                
                fragment ret(std::move(get_fragment_metadata()), std::move(data));
                
                transmitted_fragment_id = ret.object_id();
                transfer_state = state::WAITING;

                return ret;
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
                current_fragment = pos;

                /* resize the data() so it reflects the actual size now that we have received
                the last fragment, more on that in the receive constructor comment */
                if (pos == fragments_total)
                    data().shrink(0, expected_max_size - f.data().size());

                return true;
            }

            bool retransmit_request(index_type pos)
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

            /* call this when the interface acknowledges the transmission of sent fragment */
            bool fragment_transmitted()
            {
                if (is_fragment_transmitted())
                {
                    transfer_state = state::SENT;
                    last_tx_time = clock::now();

                    return true;
                }
                return false;
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

            inline bool is_current_first() const
            {
                return current_fragment == 1;
            }

            inline bool is_current_last() const
            {
                return current_fragment == fragments_total;
            }

            inline bool is_current_bordering() const
            {
                return is_current_first() || is_current_last();
            }

            inline bool is_current_main() const
            {
                return current_fragment > 1 && current_fragment < fragments_total;
            }

            /* for outgoing transfers, check if a fragment of this should be transmitted */
            inline bool is_transmit_ready() const
            {
                return transfer_state == state::NEW || transfer_state == state::NEXT || transfer_state == state::RETRY;
            }

            /* for outgoing and incoming transfers, true after a successful call 
            of get_fragment(), get_current_fragment() or get_ack_fragment() */
            inline bool is_fragment_transmitted() const
            {
                return transfer_state == state::WAITING;
            }

            /* for outgoing transfers, check if the transmit confirmation from interface belongs to this transfer */
            inline bool is_fragment_transmitted_of(object_id_type id) const
            {
                return is_fragment_transmitted() && transmitted_fragment_id == id;
            }
            
            /* for incoming transfers, check if a fragment ACK should be transmitted this transfer,
            this should also be combined with some transmit hold-off */
            inline bool is_ack_ready() const
            {
                return is_current_bordering() && transfer_state == state::NEXT;
            }


            /* fragment_transmitted() update this time */
            inline auto get_last_tx_time() const
            {
                return last_tx_time;
            }

            /* put_fragment() update this time */
            inline auto get_last_rx_time() const
            {
                return last_rx_time;
            }

            inline auto get_max_fragment_data_size() const
            {
                return max_fragment_size;
            }

            inline auto get_fragments_total() const
            {
                return fragments_total;
            }

            protected:
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
            object_id_type transmitted_fragment_id;
            
            inline data_type::iterator fragment_data_begin(index_type pos)
            {
                return data().begin() + ((pos - 1) * max_fragment_size);
            }
            inline void increment_fragment_status(index_type pos)
            {
                fragment_status.at(pos - 1) += 1;
            }
            inline Header create_header(message_types type, index_type pos) const
            {
                return Header(type, pos, fragments_total, get_id(), get_prev_id(), 0);
            }
            /* if this returns true, get the current fragment and transmit it */
            bool prepare_for_transmit()
            {
                if (is_transmit_ready())
                {
                    if (transfer_state != state::RETRY && current_fragment < fragments_total)
                        ++current_fragment;

                    transfer_state = state::WAITING;
                    return true;
                }
                return false;
            }
        };
    }
}


#endif
