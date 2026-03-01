package com.uuidgenerator.lib.sonyflake;

import com.uuidgenerator.lib.IdGenerator;
import com.uuidgenerator.lib.NetworkUtil;

import java.util.concurrent.atomic.AtomicLong;

public class Sonyflake implements IdGenerator {
    private static final long EPOCH = 1767225600000L; // Jan 1, 2026
    private static final long MACHINE_ID_BITS = 16L;
    private static final long SEQUENCE_BITS = 8L;
    private static final long MAX_MACHINE_ID = (1L << MACHINE_ID_BITS) - 1L;
    private static final long MAX_SEQUENCE = (1L << SEQUENCE_BITS) - 1L;
    private static final long MACHINE_ID_SHIFT = SEQUENCE_BITS;
    private static final long TIMESTAMP_SHIFT = SEQUENCE_BITS + MACHINE_ID_BITS;

    private final long machineId;
    private final AtomicLong sequence = new AtomicLong(0L);
    private final AtomicLong lastTimestamp = new AtomicLong(-1L);

    public Sonyflake() {
        this.machineId = NetworkUtil.getNodeIdFromIp() & MAX_MACHINE_ID;
    }

    private long currentTime10ms() {
        return System.currentTimeMillis() / 10L;
    }

    private long waitForNext10ms(long lastTs) {
        long timestamp = currentTime10ms();
        while (timestamp <= lastTs) {
            timestamp = currentTime10ms();
        }
        return timestamp;
    }

    public long nextId() {
        long timestamp = currentTime10ms();
        long lastTs = lastTimestamp.get();

        if (timestamp < lastTs) {
            System.err.println("Clock moved backwards. Refusing to generate id.");
            return 0L;
        }

        if (timestamp == lastTs) {
            long seq = (sequence.incrementAndGet()) & MAX_SEQUENCE;
            if (seq == 0) {
                timestamp = waitForNext10ms(lastTs);
            }
        } else {
            sequence.set(0L);
        }

        lastTimestamp.set(timestamp);

        return ((timestamp - (EPOCH / 10L)) << TIMESTAMP_SHIFT) |
               (machineId << MACHINE_ID_SHIFT) |
               sequence.get();
    }

    @Override
    public String nextIdString() {
        return Long.toUnsignedString(nextId());
    }
}
