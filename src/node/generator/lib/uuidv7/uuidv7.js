const crypto = require('crypto');

class UuidV7Generator {
    async nextIdString() {
        const uuid = Buffer.alloc(16);

        // 1. Get current Unix timestamp in milliseconds (48 bits)
        const timestampMs = BigInt(Date.now());

        // 2. Fill the first 6 bytes with the timestamp
        uuid[0] = Number((timestampMs >> 40n) & 0xFFn);
        uuid[1] = Number((timestampMs >> 32n) & 0xFFn);
        uuid[2] = Number((timestampMs >> 24n) & 0xFFn);
        uuid[3] = Number((timestampMs >> 16n) & 0xFFn);
        uuid[4] = Number((timestampMs >> 8n) & 0xFFn);
        uuid[5] = Number(timestampMs & 0xFFn);

        // 3. Fill the remaining 10 bytes with random data
        crypto.randomFillSync(uuid, 6, 10);

        // 4. Set version (7) and variant (RFC4122)
        uuid[6] = (uuid[6] & 0x0F) | 0x70; // Version 7
        uuid[8] = (uuid[8] & 0x3F) | 0x80; // Variant 10

        // Format as UUID string
        return [
            uuid.toString('hex', 0, 4),
            uuid.toString('hex', 4, 6),
            uuid.toString('hex', 6, 8),
            uuid.toString('hex', 8, 10),
            uuid.toString('hex', 10, 16)
        ].join('-');
    }
}

module.exports = UuidV7Generator;
