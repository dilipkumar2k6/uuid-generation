#include "insta_snowflake.h"
#include "../network_util.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace std;

InstaSnowflake::InstaSnowflake() : shard_id(get_node_id_from_ip(MAX_INSTA_SHARD_ID)) {}

uint64_t InstaSnowflake::current_time_millis() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}

/**
 * Spin-waits until the physical clock advances past the given timestamp.
 * Throws a runtime_error if the clock moves backwards.
 */
uint64_t InstaSnowflake::wait_for_next_millis(uint64_t last_ts) {
    uint64_t timestamp = current_time_millis();
    while (timestamp <= last_ts) {
        if (timestamp < last_ts) {
            cerr << "Clock moved backwards. Refusing to generate id." << endl;
            throw runtime_error("Clock moved backwards");
        }
        timestamp = current_time_millis();
    }
    return timestamp;
}

uint64_t InstaSnowflake::next_id() {
    uint64_t timestamp = current_time_millis();
    uint64_t last_ts = last_timestamp.load();

    // Handle clock moving backwards (fail-fast)
    if (timestamp < last_ts) {
        cerr << "Clock moved backwards. Refusing to generate id." << endl;
        return 0;
    }

    // If multiple requests arrive in the same millisecond, increment sequence
    if (timestamp == last_ts) {
        uint64_t seq = (sequence.fetch_add(1) + 1) & MAX_INSTA_SEQUENCE;
        // If sequence overflows (e.g., > 1023), wait for the next millisecond
        if (seq == 0) {
            timestamp = wait_for_next_millis(last_ts);
        }
    } else {
        // Reset sequence for a new millisecond
        sequence.store(0);
    }

    // Update the last used timestamp
    last_timestamp.store(timestamp);

    // Pack the timestamp, shard ID, and sequence into a 64-bit integer
    // Layout: [1 bit unused] - [41 bits time] - [13 bits shard] - [10 bits seq]
    uint64_t id = ((timestamp - EPOCH) << INSTA_TIMESTAMP_SHIFT) |
                  (shard_id << INSTA_SHARD_ID_SHIFT) |
                  sequence.load();
    
    return id;
}
