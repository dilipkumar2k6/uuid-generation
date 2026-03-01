const { getNodeIdFromIp } = require('../network_util');

class InstaSnowflake {
    constructor() {
        this.EPOCH = 1767225600000n; // Jan 1, 2026
        this.SHARD_ID_BITS = 13n;
        this.SEQUENCE_BITS = 10n;
        this.MAX_SHARD_ID = (1n << this.SHARD_ID_BITS) - 1n;
        this.MAX_SEQUENCE = (1n << this.SEQUENCE_BITS) - 1n;
        this.SHARD_ID_SHIFT = this.SEQUENCE_BITS;
        this.TIMESTAMP_SHIFT = this.SEQUENCE_BITS + this.SHARD_ID_BITS;

        this.shardId = BigInt(getNodeIdFromIp()) & this.MAX_SHARD_ID;
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
                   (this.shardId << this.SHARD_ID_SHIFT) |
                   this.sequence;

        return id.toString();
    }
}

module.exports = InstaSnowflake;
