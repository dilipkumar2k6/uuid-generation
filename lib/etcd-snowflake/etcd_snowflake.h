#ifndef ETCD_SNOWFLAKE_H
#define ETCD_SNOWFLAKE_H

#include <cstdint>
#include <atomic>
#include <string>
#include "../id_generator.h"

class EtcdSnowflake : public IdGenerator {
private:
    uint64_t node_id;
    std::atomic<uint64_t> sequence{0};
    std::atomic<uint64_t> last_timestamp{0};
    std::string etcd_endpoint;
    std::string lease_id;

    uint64_t current_time_millis();
    uint64_t wait_for_next_millis(uint64_t last_ts);
    
    // Etcd communication methods
    std::string http_post(const std::string& url, const std::string& data);
    uint64_t claim_node_id();
    void keep_alive_lease();

public:
    EtcdSnowflake();
    ~EtcdSnowflake();
    uint64_t next_id() override;
};

#endif // ETCD_SNOWFLAKE_H
