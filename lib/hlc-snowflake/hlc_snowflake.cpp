#include "hlc_snowflake.h"
#include "../network_util.h"
#include <chrono>

using namespace std;

HlcSnowflake::HlcSnowflake() : node_id(get_node_id_from_ip() & MAX_NODE_ID) {
    // Initialize state with current time
    uint64_t pt = current_time_millis();
    state.store(pt << SEQUENCE_BITS);
}

uint64_t HlcSnowflake::current_time_millis() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t HlcSnowflake::next_id() {
    uint64_t current_state = state.load();
    uint64_t next_state;
    uint64_t next_pt;
    uint64_t next_seq;

    // Lock-free Compare-And-Swap (CAS) loop
    do {
        // Unpack current logical timestamp and sequence
        uint64_t last_pt = current_state >> SEQUENCE_BITS;
        uint64_t seq = current_state & MAX_SEQUENCE;
        
        // Get current physical time
        uint64_t pt = current_time_millis();

        if (pt > last_pt) {
            // Physical time advanced normally: update timestamp, reset sequence
            next_pt = pt;
            next_seq = 0;
        } else {
            // Physical time is the same or moved backwards (clock skew):
            // Ignore physical time, increment sequence instead
            next_pt = last_pt;
            next_seq = seq + 1;
            
            // If sequence overflows (e.g., > 4095), artificially advance logical time
            if (next_seq > MAX_SEQUENCE) {
                next_pt++;
                next_seq = 0;
            }
        }
        
        // Pack the new logical timestamp and sequence into the state
        next_state = (next_pt << SEQUENCE_BITS) | next_seq;
        
    // Attempt to atomically update the state. If another thread beat us to it,
    // current_state is updated with the new value, and we loop again.
    } while (!state.compare_exchange_weak(current_state, next_state));

    // Pack the logical timestamp, node ID, and sequence into a 64-bit integer
    // Layout: [1 bit unused] - [41 bits time] - [10 bits node] - [12 bits seq]
    uint64_t id = ((next_pt - EPOCH) << TIMESTAMP_SHIFT) |
                  (node_id << NODE_ID_SHIFT) |
                  next_seq;
    
    return id;
}
