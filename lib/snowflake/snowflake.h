#ifndef SNOWFLAKE_H
#define SNOWFLAKE_H

#include <cstdint>
#include <atomic>
#include "../id_generator.h"

class Snowflake : public IdGenerator {
private:
    uint64_t node_id;
    std::atomic<uint64_t> sequence{0};
    std::atomic<uint64_t> last_timestamp{0};

    uint64_t current_time_millis();
    uint64_t wait_for_next_millis(uint64_t last_ts);

public:
    Snowflake();
    uint64_t next_id() override;
};

#endif // SNOWFLAKE_H
