# UUID Version 7 Generator Implementation Details

This directory contains the C++ implementation of a standard UUID Version 7 generator, which provides time-ordered 128-bit unique identifiers.

## What is UUID Version 7?

UUIDv7 is a relatively new standard designed to combine the best aspects of Snowflake IDs (time-ordered, sortable) with the standard 128-bit UUID format.

Like UUIDv4, it is a 128-bit value formatted as a 36-character hexadecimal string (`8-4-4-4-12`). However, instead of being entirely random, the first 48 bits are a Unix timestamp in milliseconds.

## Design

```text
+-----------------------------------------------------------------------+
|                          128-bit UUIDv7                               |
+-----------------------------------+----+--------------------+----+----+
|              48 bits              | 4  |      12 bits       | 2  | 62 |
|             Unix Ts               |bits|    Random Data     |bits|bits|
|     (ms since standard epoch)     |Ver |      (rand_a)      |Var |Rand|
|                                   |(7) |                    |(10)|(b) |
+-----------------------------------+----+--------------------+----+----+
```

## The 128-bit Structure

The UUIDv7 bits are distributed as follows:
- **48 bits**: `unix_ts_ms` (Unix timestamp in milliseconds since the standard 1970 epoch).
- **4 bits**: `ver` (Version, always `0111` or `7`).
- **12 bits**: `rand_a` (Pseudo-random data).
- **2 bits**: `var` (Variant, always `10`).
- **62 bits**: `rand_b` (Pseudo-random data).

## Advantages over UUIDv4

1.  **Database Friendly**: Because the first 48 bits are a timestamp, UUIDv7s generated sequentially will sort sequentially. This drastically reduces index fragmentation and improves insert performance in databases (like PostgreSQL or MySQL) compared to the completely random UUIDv4.
2.  **No Custom Epoch**: It uses the standard Unix epoch, meaning the timestamp can be easily extracted and converted to a human-readable date without knowing a custom application epoch.

## Implementation Details

- **Randomness**: The generator uses `std::mt19937_64` (a 64-bit Mersenne Twister pseudo-random number generator) seeded by `std::random_device` to generate the 74 bits of required randomness (`rand_a` and `rand_b`).
- **Thread Safety**: Because `std::mt19937_64` is not thread-safe, a `std::mutex` is used to lock the random number generation step.
- **String Output**: Like UUIDv4, this generator overrides the `next_id_string()` method of the `IdGenerator` interface to return the formatted string directly.

## Pros and Cons

### Pros
*   **Time-Ordered**: The 48-bit Unix timestamp ensures IDs are roughly sortable by time, significantly improving database insert performance compared to UUIDv4.
*   **High Collision Resistance**: The 74 bits of randomness provide excellent collision resistance without requiring node coordination.
*   **Decentralized**: No central coordinator or node ID assignment is needed.
*   **Standardized**: A formal IETF standard (RFC 9562).

### Cons
*   **Storage Inefficiency**: 128-bit size requires twice the storage of 64-bit integers.
*   **Clock Dependency**: Relies on the system clock for the timestamp portion.
*   **Predictability**: The timestamp portion makes the ID somewhat predictable, which might be undesirable for certain security-sensitive applications (e.g., session tokens).
