#include "uuidv4_generator.h"
#include <sstream>
#include <iomanip>

UuidV4Generator::UuidV4Generator() : gen(rd()) {}

std::string UuidV4Generator::next_id_string() {
    uint64_t part1;
    uint64_t part2;

    {
        // Lock the generator to ensure thread safety for random generation
        // std::mt19937_64 is not thread-safe by default
        std::lock_guard<std::mutex> lock(mtx);
        part1 = dis(gen);
        part2 = dis(gen);
    }

    // ---------------------------------------------------------
    // Set Version (4) and Variant (RFC 4122) bits
    // ---------------------------------------------------------
    
    // Set version to 4 in part1 (the 13th hex character)
    // part1 layout: [32 bits] - [16 bits] - [16 bits (version + 12 bits)]
    part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    
    // Set variant to 10xx in part2 (the 17th hex character)
    // part2 layout: [16 bits (variant + 12 bits)] - [48 bits]
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

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
