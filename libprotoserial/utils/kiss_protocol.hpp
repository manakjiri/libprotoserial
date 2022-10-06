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

#ifndef _SP_KISSPROTOCOL
#define _SP_KISSPROTOCOL

#include "libprotoserial/libconfig.hpp"
#include "libprotoserial/clock.hpp"
#include "libprotoserial/interface.hpp"

#include <list>

namespace sp
{

/*
 * The simplest practical protocol working directly above the interface layer.
 * Exists mostly for testing purposes.
 * It implements rudimentary ACK and keep-alive system.
 *
 * Allowed type range is 0 to 250, the rest is reserved.
 */
class kiss_protocol
{
    enum
    {
    ACK_TYPE,
    PING_TYPE,
    TYPE_OFFSET = 5
    };

    struct data_handle
    {
        byte type;
        byte id;
        bytes data;
        fragment::address_type addr;

        data_handle(byte type, bytes data, fragment::address_type addr) :
            type(type), id(++id_counter), data(std::move(data)), 
            addr(addr), retries(0), sent_at(clock::now()) {}

        void sent()
        {
            sent_at = clock::now();
            ++retries;
        }

        bool transmit_expired(clock::duration holdoff) const
        {
            return sent_at + holdoff <= clock::now();
        }

        bool retries_exhausted(uint retries_max) const
        {
            return retries >= retries_max;
        }

        private:

        uint retries;
        clock::time_point sent_at;
        static inline uint8_t id_counter = 0;
    };

    std::list<data_handle> packet_list;
    interface & _interface;

    uint retries_max;
    clock::duration retry_holdoff;
    fragment::address_type default_addr;

    clock::duration ping_holdoff {};
    clock::time_point last_transmitted {clock::now()}, last_received = never();

    void receive(fragment f)
    {
        if (f.data().size() >= 2)
        {
            byte type = f.data().at(0);
            byte id = f.data().at(1);

            last_received = clock::now();

            if (type >= TYPE_OFFSET)
            {
                last_transmitted = clock::now();

                f.data().shrink(2, 0);
                /* transmit the ACK fragment */
                fragment resp(f.source(), bytes({(byte)ACK_TYPE, id}));
                _interface.transmit(std::move(resp));

                receive_event.emit((uint8_t)type - TYPE_OFFSET, std::move(f.data()));
            }
            else
            {
                /* we have received an ACK for this ID, remove it from the list */
                std::remove_if(packet_list.begin(), packet_list.end(),
                        [id](const auto & p){return p.id == id;});
            }
        }
    }

    bool transmit_packet(const data_handle & p)
    {
        last_transmitted = clock::now();

        auto data = _interface.minimum_prealloc().create(0, 2, p.data.size());
        data[0] = p.type;
        data[1] = p.id;
        data.push_back(const_cast<const bytes&>(p.data));

        _interface.transmit(fragment(p.addr, std::move(data)));
        return true;
    }

    public:

    subject<uint8_t, bytes> receive_event;

    kiss_protocol(interface & i, clock::duration retry, uint retries = 3):
        _interface(i), retries_max(retries), retry_holdoff(retry), default_addr(0)
    {
        _interface.receive_event.subscribe(&kiss_protocol::receive, this);
    }

    void set_default_addr(fragment::address_type addr)
    {
        default_addr = addr;
    }

    void enable_keepalive_ping(clock::duration max_silence_duration)
    {
        ping_holdoff = max_silence_duration;
    }

    void transmit(uint8_t type, bytes data)
    {
        auto & p = packet_list.emplace_back(type + TYPE_OFFSET, std::move(data), default_addr);
        
        if (transmit_packet(p))
            p.sent();
        else
            packet_list.pop_back();
    }

    void main_task()
    {
        for (auto it = packet_list.begin(); it != packet_list.end(); ++it)
        {
            if (it->transmit_expired(retry_holdoff))
            {
                if (!it->retries_exhausted(retries_max))
                {
                    if (transmit_packet(*it))
                    {
                        it->sent();
                        return;
                    }
                    else
                        it = packet_list.erase(it);
                }
                else
                    it = packet_list.erase(it);
            }
        }

        if (ping_holdoff != clock::duration{} && last_transmitted + ping_holdoff <= clock::now())
        {
            auto & p = packet_list.emplace_back(PING_TYPE, bytes(), default_addr);

            if (transmit_packet(p))
                p.sent();
            else
                packet_list.pop_back();
        }
    }

    auto pending_count() const
    {
        return packet_list.size();
    }

    bool is_peer_alive() const
    {
        return last_received + (retries_max * retry_holdoff) > clock::now();
    }

};

}

#endif
