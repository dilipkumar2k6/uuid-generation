package com.uuidgenerator.lib.instasnowflake;

import com.uuidgenerator.lib.IdGenerator;
import com.uuidgenerator.lib.NetworkUtil;

import java.util.concurrent.atomic.AtomicLong;

public class InstaSnowflake implements IdGenerator {
    private static final long EPOCH = 1767225600000L; // Jan 1, 2026
    private static final long SHARD_ID_BITS = 13L;
    private static final long SEQUENCE_BITS = 10L;
    private static final long MAX_SHARD_ID = (1L << SHARD_ID_BITS) - 1L;
    private static final long MAX_SEQUENCE = (1L << SEQUENCE_BITS) - 1L;
    private static final long SHARD_ID_SHIFT = SEQUENCE_BITS;
    private static final long TIMESTAMP_SHIFT = SEQUENCE_BITS + SHARD_ID_BITS;

    private final long shardId;
    private final AtomicLong sequence = new AtomicLong(0L);
    private final AtomicLong lastTimestamp = new AtomicLong(-1L);

    public InstaSnowflake() {
        this.shardId = NetworkUtil.getNodeIdFromIp() & MAX_SHARD_ID;
    }

    private long currentTimeMillis() {
        return System.currentTimeMillis();
    }

    private long waitForNextMillis(long lastTs) {
        long timestamp = currentTimeMillis();
        while (timestamp <= lastTs) {
            timestamp = currentTimeMillis();
        }
        return timestamp;
    }

    public long nextId() {
        long timestamp = currentTimeMillis();
        long lastTs = lastTimestamp.get();

        if (timestamp < lastTs) {
            System.err.println("Clock moved backwards. Refusing to generate id.");
            return 0L;
        }

        if (timestamp == lastTs) {
            long seq = (sequence.incrementAndGet()) & MAX_SEQUENCE;
            if (seq == 0) {
                timestamp = waitForNextMillis(lastTs);
            }
        } else {
            sequence.set(0L);
        }

        lastTimestamp.set(timestamp);

        return ((timestamp - EPOCH) << TIMESTAMP_SHIFT) |
               (shardId << SHARD_ID_SHIFT) |
               sequence.get();
    }

    @Override
    public String nextIdString() {
        return Long.toUnsignedString(nextId());
    }
}
