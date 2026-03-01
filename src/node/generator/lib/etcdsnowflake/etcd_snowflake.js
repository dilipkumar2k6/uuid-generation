// Note: A full Etcd implementation in Node.js requires an external library like etcd3.
// For the sake of this example and to avoid complex dependency management in a simple
// script-based build, we will provide a simplified stub that falls back to IP-based
// node ID generation if etcd is not available, or just uses a static ID for demonstration.

class EtcdSnowflake {
    constructor() {
        this.EPOCH = 1767225600000n; // Jan 1, 2026
        this.NODE_ID_BITS = 10n;
        this.SEQUENCE_BITS = 12n;
        this.MAX_NODE_ID = (1n << this.NODE_ID_BITS) - 1n;
        this.MAX_SEQUENCE = (1n << this.SEQUENCE_BITS) - 1n;
        this.NODE_ID_SHIFT = this.SEQUENCE_BITS;
        this.TIMESTAMP_SHIFT = this.SEQUENCE_BITS + this.NODE_ID_BITS;

        console.log("Warning: EtcdSnowflake in Node.js is using a stubbed Node ID (1) due to missing etcd3 dependency.");
        this.nodeId = 1n & this.MAX_NODE_ID;
        this.sequence = 0n;
        this.lastTimestamp = -1n;
    }

    currentTimeMillis() {
        return BigInt(Date.now());
    }

    waitForNextMillis(lastTs) {
        let timestamp = this.currentTimeMillis();
        while (timestamp <= lastTs) {
            timestamp = this.currentTimeMillis();
        }
        return timestamp;
    }

    async nextIdString() {
        let timestamp = this.currentTimeMillis();

        if (timestamp < this.lastTimestamp) {
            console.error("Clock moved backwards. Refusing to generate id.");
            return "0";
        }

        if (timestamp === this.lastTimestamp) {
            this.sequence = (this.sequence + 1n) & this.MAX_SEQUENCE;
            if (this.sequence === 0n) {
                timestamp = this.waitForNextMillis(this.lastTimestamp);
            }
        } else {
            this.sequence = 0n;
        }

        this.lastTimestamp = timestamp;

        const id = ((timestamp - this.EPOCH) << this.TIMESTAMP_SHIFT) |
                   (this.nodeId << this.NODE_ID_SHIFT) |
                   this.sequence;

        return id.toString();
    }
}

module.exports = EtcdSnowflake;
