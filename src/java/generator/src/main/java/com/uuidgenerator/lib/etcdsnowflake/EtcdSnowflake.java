package com.uuidgenerator.lib.etcdsnowflake;

import com.uuidgenerator.lib.IdGenerator;

import java.util.concurrent.atomic.AtomicLong;

// Note: A full Etcd implementation in Java requires an external library like jetcd.
// For the sake of this example and to avoid complex dependency management in a simple
// script-based build, we will provide a simplified stub that falls back to IP-based
// node ID generation if etcd is not available, or just uses a static ID for demonstration.
// In a real production Java application, you would use Maven/Gradle and include jetcd.

public class EtcdSnowflake implements IdGenerator {
    private static final long EPOCH = 1767225600000L; // Jan 1, 2026
    private static final long NODE_ID_BITS = 10L;
    private static final long SEQUENCE_BITS = 12L;
    private static final long MAX_NODE_ID = (1L << NODE_ID_BITS) - 1L;
    private static final long MAX_SEQUENCE = (1L << SEQUENCE_BITS) - 1L;
    private static final long NODE_ID_SHIFT = SEQUENCE_BITS;
    private static final long TIMESTAMP_SHIFT = SEQUENCE_BITS + NODE_ID_BITS;

    private final long nodeId;
    private final AtomicLong sequence = new AtomicLong(0L);
    private final AtomicLong lastTimestamp = new AtomicLong(-1L);

    public EtcdSnowflake() {
        // Simplified: In a real app, connect to etcd here.
        // For this demo, we'll just use a static node ID to represent the concept.
        System.out.println("Warning: EtcdSnowflake in Java is using a stubbed Node ID (1) due to missing jetcd dependency.");
        this.nodeId = 1L & MAX_NODE_ID;
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
               (nodeId << NODE_ID_SHIFT) |
               sequence.get();
    }

    @Override
    public String nextIdString() {
        return Long.toUnsignedString(nextId());
    }
}
