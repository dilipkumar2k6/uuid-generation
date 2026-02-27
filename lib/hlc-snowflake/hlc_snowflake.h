#ifndef HLC_SNOWFLAKE_H
#define HLC_SNOWFLAKE_H

#include <cstdint>
#include <atomic>
#include "../id_generator.h"

class HlcSnowflake : public IdGenerator {
private:
    uint64_t node_id;
    // state packs the 41-bit timestamp and 12-bit sequence into a single 64-bit atomic
    std::atomic<uint64_t> state{0}; 

    uint64_t current_time_millis();

public:
    HlcSnowflake();
    uint64_t next_id() override;
};

#endif // HLC_SNOWFLAKE_H
