# Snowflake
Snowflake is a network service for generating unique ID numbers at high scale with some simple guarantees.
# Motivation
As we at Twitter move away from Mysql towards Cassandra, we've needed a new way to generate id numbers. There is no sequential id generation facility in Cassandra, nor should there be.
# Requirements
## Performance
- minimum 10k ids per second per process
- response rate 2ms (plus network latency)
## Uncoordinated
For high availability within and across data centers, machines generating ids should not have to coordinate with each other.

## (Roughly) Time Ordered
## Directly Sortable
The ids should be sortable without loading the full objects that the represent. This sorting should be the above ordering.

## Compact
There are many otherwise reasonable solutions to this problem that require 128bit numbers. For various reasons, we need to keep our ids under 64bits.

## Highly Available
The id generation scheme should be at least as available as our related services (like our storage services).

# Solution
id is composed of
- time - 41 bits (millisecond precision w/ a custom epoch gives us 69 years)
- configured machine id - 10 bits - gives us up to 1024 machines
- sequence number - 12 bits - rolls over every 4096 per machine (with protection to avoid rollover in the same ms)
# System Clock Dependency
- You should use NTP to keep your system clock accurate.