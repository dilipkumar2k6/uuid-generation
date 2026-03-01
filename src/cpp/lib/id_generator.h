#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include <cstdint>
#include <string>

// ---------------------------------------------------------
// Shared Parameters for 64-bit ID Generators
// ---------------------------------------------------------
const uint64_t EPOCH = 1767225600000ULL;  // Jan 1, 2026
const uint64_t NODE_ID_BITS = 10;
const uint64_t SEQUENCE_BITS = 12;

// Max values for bitwise operations
const uint64_t MAX_NODE_ID = (static_cast<uint64_t>(1) << NODE_ID_BITS) - 1;
const uint64_t MAX_SEQUENCE = (static_cast<uint64_t>(1) << SEQUENCE_BITS) - 1;

// Bit shifts for packing the 64-bit ID
const uint64_t NODE_ID_SHIFT = SEQUENCE_BITS;
const uint64_t TIMESTAMP_SHIFT = SEQUENCE_BITS + NODE_ID_BITS;

/**
 * Base interface for all ID generators.
 */
class IdGenerator {
 public:
  virtual ~IdGenerator() = default;

  // Returns the ID as a raw 64-bit integer (if applicable)
  virtual uint64_t next_id() { return 0; }

  // Returns the ID as a formatted string (used for IPC)
  virtual std::string next_id_string() { return std::to_string(next_id()); }
};

#endif  // ID_GENERATOR_H
