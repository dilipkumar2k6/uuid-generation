#ifndef SONYFLAKE_H
#define SONYFLAKE_H

#include <atomic>
#include <cstdint>

#include "../id_generator.h"

// Sonyflake specific parameters
const uint64_t SONY_TIME_BITS = 39;
const uint64_t SONY_SEQUENCE_BITS = 8;
const uint64_t SONY_MACHINE_ID_BITS = 16;

const uint64_t MAX_SONY_SEQUENCE =
    (static_cast<uint64_t>(1) << SONY_SEQUENCE_BITS) - 1;
const uint64_t MAX_SONY_MACHINE_ID =
    (static_cast<uint64_t>(1) << SONY_MACHINE_ID_BITS) - 1;

const uint64_t SONY_MACHINE_ID_SHIFT = 0;
const uint64_t SONY_SEQUENCE_SHIFT = SONY_MACHINE_ID_BITS;
const uint64_t SONY_TIMESTAMP_SHIFT = SONY_SEQUENCE_BITS + SONY_MACHINE_ID_BITS;

// Sonyflake uses 10ms units instead of 1ms
const uint64_t SONY_EPOCH_10MS = EPOCH / 10;

class Sonyflake : public IdGenerator {
 private:
  uint64_t machine_id;
  std::atomic<uint64_t> sequence{0};
  std::atomic<uint64_t> last_timestamp{0};

  uint64_t current_time_10ms();
  uint64_t wait_for_next_10ms(uint64_t last_ts);

 public:
  Sonyflake();
  uint64_t next_id() override;
};

#endif  // SONYFLAKE_H
