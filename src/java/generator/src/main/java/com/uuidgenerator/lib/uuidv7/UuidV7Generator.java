package com.uuidgenerator.lib.uuidv7;

import com.uuidgenerator.lib.IdGenerator;

import java.security.SecureRandom;

public class UuidV7Generator implements IdGenerator {
    private final SecureRandom random = new SecureRandom();

    @Override
    public String nextIdString() {
        byte[] uuid = new byte[16];

        // 1. Get current Unix timestamp in milliseconds (48 bits)
        long timestampMs = System.currentTimeMillis();

        // 2. Fill the first 6 bytes with the timestamp
        uuid[0] = (byte) ((timestampMs >> 40) & 0xFF);
        uuid[1] = (byte) ((timestampMs >> 32) & 0xFF);
        uuid[2] = (byte) ((timestampMs >> 24) & 0xFF);
        uuid[3] = (byte) ((timestampMs >> 16) & 0xFF);
        uuid[4] = (byte) ((timestampMs >> 8) & 0xFF);
        uuid[5] = (byte) (timestampMs & 0xFF);

        // 3. Fill the remaining 10 bytes with random data
        byte[] randomBytes = new byte[10];
        random.nextBytes(randomBytes);
        System.arraycopy(randomBytes, 0, uuid, 6, 10);

        // 4. Set version (7) and variant (RFC4122)
        uuid[6] = (byte) ((uuid[6] & 0x0F) | 0x70); // Version 7
        uuid[8] = (byte) ((uuid[8] & 0x3F) | 0x80); // Variant 10

        return String.format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                uuid[0], uuid[1], uuid[2], uuid[3],
                uuid[4], uuid[5],
                uuid[6], uuid[7],
                uuid[8], uuid[9],
                uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    }
}
