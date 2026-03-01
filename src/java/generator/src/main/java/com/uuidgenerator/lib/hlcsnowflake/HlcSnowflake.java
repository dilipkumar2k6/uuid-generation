package com.uuidgenerator.lib.hlcsnowflake;

import com.uuidgenerator.lib.IdGenerator;
import com.uuidgenerator.lib.NetworkUtil;

import java.util.concurrent.atomic.AtomicLong;

public class HlcSnowflake implements IdGenerator {
    private static final long EPOCH = 1767225600000L; // Jan 1, 2026
    private static final long NODE_ID_BITS = 10L;
    private static final long SEQUENCE_BITS = 12L;
    private static final long MAX_NODE_ID = (1L << NODE_ID_BITS) - 1L;
    private static final long MAX_SEQUENCE = (1L << SEQUENCE_BITS) - 1L;
    private static final long NODE_ID_SHIFT = SEQUENCE_BITS;
    private static final long TIMESTAMP_SHIFT = SEQUENCE_BITS + NODE_ID_BITS;

    private final long nodeId;
    private final AtomicLong logicalTime = new AtomicLong(0L);
    private final AtomicLong sequence = new AtomicLong(0L);

    public HlcSnowflake() {
        this.nodeId = NetworkUtil.getNodeIdFromIp() & MAX_NODE_ID;
    }

    private long physicalTimeMillis() {
        return System.currentTimeMillis();
    }

    public long nextId() {
        long physicalTime = physicalTimeMillis();
        long currentLogicalTime = logicalTime.get();

        long newLogicalTime;

        if (physicalTime > currentLogicalTime) {
            newLogicalTime = physicalTime;
            logicalTime.set(newLogicalTime);
            sequence.set(0L);
        } else {
            newLogicalTime = currentLogicalTime;
            long seq = (sequence.incrementAndGet()) & MAX_SEQUENCE;
            if (seq == 0) {
                newLogicalTime = logicalTime.incrementAndGet();
            }
        }

        return ((newLogicalTime - EPOCH) << TIMESTAMP_SHIFT) |
               (nodeId << NODE_ID_SHIFT) |
               sequence.get();
    }

    @Override
    public String nextIdString() {
        return Long.toUnsignedString(nextId());
    }
}
