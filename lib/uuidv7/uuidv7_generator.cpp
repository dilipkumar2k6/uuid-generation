#include "uuidv7_generator.h"
#include <sstream>
#include <iomanip>
#include <chrono>

UuidV7Generator::UuidV7Generator() : gen(rd()) {}

uint64_t UuidV7Generator::current_time_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string UuidV7Generator::next_id_string() {
    uint64_t timestamp = current_time_millis();
    uint64_t rand_a;
    uint64_t rand_b;

    {
        // Lock the generator to ensure thread safety for random generation
        // std::mt19937_64 is not thread-safe by default
        std::lock_guard<std::mutex> lock(mtx);
        rand_a = dis(gen);
        rand_b = dis(gen);
    }

    // ---------------------------------------------------------
    // Set Version (7) and Variant (RFC 4122) bits
    // ---------------------------------------------------------
    // UUIDv7 Layout:
    // unix_ts_ms: 48 bits (from timestamp)
    // ver: 4 bits (0111)
    // rand_a: 12 bits
    // var: 2 bits (10)
    // rand_b: 62 bits

    // part1 contains: unix_ts_ms (48 bits) | ver (4 bits) | rand_a (12 bits)
    uint64_t part1 = (timestamp << 16) | 0x7000ULL | (rand_a & 0x0FFFULL);
    
    // part2 contains: var (2 bits) | rand_b (62 bits)
    uint64_t part2 = 0x8000000000000000ULL | (rand_b & 0x3FFFFFFFFFFFFFFFULL);

    // ---------------------------------------------------------
    // Format as 8-4-4-4-12 hex string
    // ---------------------------------------------------------
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (part1 >> 32) << "-"
       << std::setw(4) << ((part1 >> 16) & 0xFFFF) << "-"
       << std::setw(4) << (part1 & 0xFFFF) << "-"
       << std::setw(4) << (part2 >> 48) << "-"
       << std::setw(12) << (part2 & 0xFFFFFFFFFFFFULL);
    
    return ss.str();
}
