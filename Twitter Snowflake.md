# Twitter Snowflake
A dedicated network service for generating 64-bit unique IDs at high scale. The IDs are made up of the following components:
- Epoch timestamp in millisecond precision - 41 bits (gives us 69 years with a custom epoch)
- Configured machine id - 10 bits (gives us up to 1024 machines)
- Sequence number - 12 bits (A local counter per machine that rolls over every 4096)
- The extra 1 bit is reserved for future purposes. Since the IDs use timestamp as the first component, they are time sortable.

