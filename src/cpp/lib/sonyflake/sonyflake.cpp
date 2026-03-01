#include "sonyflake.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

#include "../network_util.h"

using namespace std;

Sonyflake::Sonyflake() : machine_id(get_node_id_from_ip(MAX_SONY_MACHINE_ID)) {}

uint64_t Sonyflake::current_time_10ms() {
  // Sonyflake uses 10ms units instead of 1ms
  return chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
             .count() /
         10;
}

/**
 * Spin-waits until the physical clock advances past the given timestamp.
 * Throws a runtime_error if the clock moves backwards.
 */
uint64_t Sonyflake::wait_for_next_10ms(uint64_t last_ts) {
  uint64_t timestamp = current_time_10ms();
  while (timestamp <= last_ts) {
    if (timestamp < last_ts) {
      cerr << "Clock moved backwards. Refusing to generate id." << endl;
      throw runtime_error("Clock moved backwards");
    }
    timestamp = current_time_10ms();
  }
  return timestamp;
}

uint64_t Sonyflake::next_id() {
  uint64_t timestamp = current_time_10ms();
  uint64_t last_ts = last_timestamp.load();

  // Handle clock moving backwards (fail-fast)
  if (timestamp < last_ts) {
    cerr << "Clock moved backwards. Refusing to generate id." << endl;
    return 0;
  }

  // If multiple requests arrive in the same 10ms unit, increment sequence
  if (timestamp == last_ts) {
    uint64_t seq = (sequence.fetch_add(1) + 1) & MAX_SONY_SEQUENCE;
    // If sequence overflows (e.g., > 255), wait for the next 10ms unit
    if (seq == 0) {
      timestamp = wait_for_next_10ms(last_ts);
    }
  } else {
    // Reset sequence for a new 10ms unit
    sequence.store(0);
  }

  // Update the last used timestamp
  last_timestamp.store(timestamp);

  // Pack the timestamp, sequence, and machine ID into a 64-bit integer
  // Layout: [1 bit unused] - [39 bits time] - [8 bits seq] - [16 bits machine]
  // Note: Sonyflake order is Time -> Sequence -> Machine ID
  uint64_t id = ((timestamp - SONY_EPOCH_10MS) << SONY_TIMESTAMP_SHIFT) |
                (sequence.load() << SONY_SEQUENCE_SHIFT) |
                (machine_id << SONY_MACHINE_ID_SHIFT);

  return id;
}
