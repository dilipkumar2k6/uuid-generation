# Sonyflake ID Generator Implementation Details

This directory contains the implementation of the Sonyflake Variation of the Snowflake ID generation algorithm. This variation was originally created by Sony.

## What is the Sonyflake Variation?

While the standard Snowflake algorithm uses a 41-bit timestamp (1ms units), 10-bit Node ID, and 12-bit Sequence, Sonyflake modifies this distribution to prioritize a longer lifespan and more distributed nodes, at the cost of peak single-node throughput.

The 64-bit ID structure is distributed as follows:
- **1 bit**: Unused (always `0`).
- **39 bits**: Timestamp (in **10ms** units since `EPOCH`).
- **8 bits**: Sequence number.
- **16 bits**: Machine ID.

*Note: The order of bits in Sonyflake is typically Time -> Sequence -> Machine ID, unlike standard Snowflake which is Time -> Node ID -> Sequence.*

## Component Diagram

This diagram shows the sidecar architecture for the Sonyflake generator.

![Component Diagram](component-diagram.svg)

## Design


## Why change the bit distribution?

### 1. Longer Lifespan (39 bits @ 10ms)
By using 10ms units instead of 1ms units, the 39-bit timestamp can last for $2^{39} \times 10$ milliseconds, which equals approximately **174 years** (compared to standard Snowflake's 69 years).

### 2. More Machines (16 bits)
By allocating 16 bits to the Machine ID, the system can support up to $2^{16} = 65,536$ unique nodes/machines (compared to the standard 1024). This is ideal for massive global deployments across many small instances.

### 3. Smaller Sequence (8 bits)
To make room for the longer lifespan and more machines, the sequence is reduced to 8 bits. This means a single node can generate up to $2^8 = 256$ unique IDs per **10ms** window. This translates to a maximum throughput of **25,600 IDs per second per node**. While much lower than standard Snowflake's 4 million/sec, it is still more than sufficient for the vast majority of individual microservices.

## Implementation Details

- **Time Units**: The `current_time_10ms()` function divides the standard millisecond timestamp by 10.
- **Machine ID Derivation**: This sidecar derives its Machine ID dynamically from the last 16 bits of the container's IPv4 address.
- **Thread Safety**: The sequence and timestamp are managed using `std::atomic<uint64_t>` to ensure thread-safe, lock-free ID generation.
- **Clock Skew**: Like the standard Snowflake, this implementation uses a "fail-fast" spin-wait approach if the physical clock moves backwards.

## Flow Diagram

This flowchart explains the Sonyflake algorithm, emphasizing the use of 10ms time units and the specific bit distribution for the Machine ID and sequence.

![Flow Diagram](flow-diagram.svg)

## Sequence Diagram

This sequence diagram outlines the request flow and the bitwise operations used to construct the final Sonyflake ID.

![Sequence Diagram](sequence-diagram.svg)

## Pros and Cons

### Pros
*   **Long Lifespan**: The 39-bit timestamp (in 10ms units) extends the system's lifespan to ~174 years (vs 69 years in standard Snowflake).
*   **Massive Scale**: Supports 65,536 nodes (vs 1,024 in standard Snowflake), making it ideal for massive global deployments.
*   **Efficient Storage**: Generates 64-bit integers, which are highly optimized for database indexing and storage compared to 128-bit UUIDs.
*   **Time-Ordered**: IDs are roughly sortable by time (k-sortable), improving database insert performance.
*   **Decentralized**: No central coordinator is needed during ID generation.

### Cons
*   **Lower Throughput**: Generates up to 256 IDs per 10ms per node (25.6 IDs/ms), which is significantly lower than standard Snowflake (4,096 IDs/ms).
*   **Coarser Resolution**: The 10ms resolution might not be granular enough for extreme high-frequency trading or logging systems.
*   **Clock Dependency**: Relies heavily on the system clock (NTP synchronization required).
*   **Availability Risk**: Fails fast and refuses to generate IDs if the clock moves backwards.
*   **Node ID Coordination**: Requires a reliable mechanism to assign unique Node IDs to prevent collisions.
