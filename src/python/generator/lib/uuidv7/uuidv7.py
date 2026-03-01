import time
import os

class UuidV7Generator:
    def next_id_string(self):
        # 1. Get current Unix timestamp in milliseconds (48 bits)
        timestamp_ms = int(time.time() * 1000)

        # 2. Fill the first 6 bytes with the timestamp
        uuid_bytes = bytearray(16)
        uuid_bytes[0] = (timestamp_ms >> 40) & 0xFF
        uuid_bytes[1] = (timestamp_ms >> 32) & 0xFF
        uuid_bytes[2] = (timestamp_ms >> 24) & 0xFF
        uuid_bytes[3] = (timestamp_ms >> 16) & 0xFF
        uuid_bytes[4] = (timestamp_ms >> 8) & 0xFF
        uuid_bytes[5] = timestamp_ms & 0xFF

        # 3. Fill the remaining 10 bytes with random data
        random_bytes = os.urandom(10)
        uuid_bytes[6:16] = random_bytes

        # 4. Set version (7) and variant (RFC4122)
        uuid_bytes[6] = (uuid_bytes[6] & 0x0F) | 0x70 # Version 7
        uuid_bytes[8] = (uuid_bytes[8] & 0x3F) | 0x80 # Variant 10

        # Format as UUID string
        hex_str = uuid_bytes.hex()
        return f"{hex_str[0:8]}-{hex_str[8:12]}-{hex_str[12:16]}-{hex_str[16:20]}-{hex_str[20:32]}"
