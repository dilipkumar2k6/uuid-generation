#ifndef INSTA_SNOWFLAKE_H
#define INSTA_SNOWFLAKE_H

#include <atomic>
#include <cstdint>

#include "../id_generator.h"

// Instagram specific parameters
const uint64_t INSTA_SHARD_ID_BITS = 13;
const uint64_t INSTA_SEQUENCE_BITS = 10;

const uint64_t MAX_INSTA_SHARD_ID =
    (static_cast<uint64_t>(1) << INSTA_SHARD_ID_BITS) - 1;
const uint64_t MAX_INSTA_SEQUENCE =
    (static_cast<uint64_t>(1) << INSTA_SEQUENCE_BITS) - 1;

const uint64_t INSTA_SHARD_ID_SHIFT = INSTA_SEQUENCE_BITS;
const uint64_t INSTA_TIMESTAMP_SHIFT =
    INSTA_SEQUENCE_BITS + INSTA_SHARD_ID_BITS;

class InstaSnowflake : public IdGenerator {
 private:
  uint64_t shard_id;
  std::atomic<uint64_t> sequence{0};
  std::atomic<uint64_t> last_timestamp{0};

  uint64_t current_time_millis();
  uint64_t wait_for_next_millis(uint64_t last_ts);

 public:
  InstaSnowflake();
  uint64_t next_id() override;
};

#endif  // INSTA_SNOWFLAKE_H
