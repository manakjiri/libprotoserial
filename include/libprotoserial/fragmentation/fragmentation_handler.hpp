class peer_state
        {
            public:
            peer_state(address_type a, const configuration & c) : //TODO
                addr(a), tx_rate(c.peer_rate), last_rx(never()), tx_holdoff(clock::now()) {}

            address_type addr;
            /* from our point of view */
            uint tx_rate;
            /* from our point of view */
            clock::time_point last_rx, tx_holdoff;

            bool in_transmit_holdoff() const {return tx_holdoff > clock::now();}
        };

        auto find_peer(address_type addr)
        {
            auto peer = std::find_if(_peer_states.begin(), _peer_states.end(), [&](const peer_state & ps){
                return ps.addr == addr;
            });
            if (peer == _peer_states.end())
            {
                _peer_states.emplace_front(addr, _config);
                return _peer_states.begin();
            }
            else
                return peer;
        }


        struct configuration
        {
            /* these are ballpark estimates of the allowable tx and rx rates in bytes per second */
            bit_rate tx_rate, rx_rate;
            /* this is the initial peer transmit rate */
            bit_rate peer_rate;

            /* thresholds of the rx buffer levels (interface::status.free_receive_buffer) in bytes
            these influence our_status, frb_poor is the threshold where the status returns rx_poor() == true,
            frb_critical is when it returns rx_critical() == true */
            uint frb_poor, frb_critical;
            
            /* denominator value of the transmit rate that gets applied when the peer's
            status evaluates to rx_poor(), it triples for rx_critical() */
            double tr_decrease;
            /* counteracts tr_divider by incrementing the transmit rate when the conditions are favorable */
            uint tr_increase;
            
            /* 1 means that a retransmit request is sent immediately when it is detected that a fragment
            of such size could have been received given the rx_rate, values > 1 will increase this holdoff
            and are suitable when there are many connections at once. 
            note that this is not the only precondition */
            uint retransmit_request_holdoff_multiplier;
            /* inactivity timeout is a mechanism of purging stalled incoming or outgoing transfers 
            its initial value is derived from the fragment size and peer transmit or receive rate 
            and this multiplier */
            uint inactivity_timeout_multiplier;
            /* minimum hold duration for completed incoming transfers, it is necessary to hold onto 
            these received transfers in order to detect spurious retransmits that may happen due to
            fragment delays and lost ACKs */
            clock::duration minimum_incoming_hold_time;

            /* this tries to set good default values */
            constexpr configuration(const interface & i, bit_rate rate, size_type rx_buffer_size)
            {
                tx_rate = rx_rate = rate;
                peer_rate = tx_rate / 5;
                retransmit_request_holdoff_multiplier = 3;

                //max_fragment_data_size = i.max_data_size();
                
                inactivity_timeout_multiplier = 5;
                minimum_incoming_hold_time = peer_rate.bit_period() * rx_buffer_size;
                tr_decrease = 2;
                //tr_increase = max_fragment_data_size / 10;

                //frb_poor = max_fragment_data_size + (fragment_overhead_size * 2);
                frb_critical = frb_poor * 3;
            }
        };