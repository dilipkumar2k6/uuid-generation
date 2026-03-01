const { getNodeIdFromIp } = require('../network_util');

class HlcSnowflake {
    constructor() {
        this.EPOCH = 1767225600000n; // Jan 1, 2026
        this.NODE_ID_BITS = 10n;
        this.SEQUENCE_BITS = 12n;
        this.MAX_NODE_ID = (1n << this.NODE_ID_BITS) - 1n;
        this.MAX_SEQUENCE = (1n << this.SEQUENCE_BITS) - 1n;
        this.NODE_ID_SHIFT = this.SEQUENCE_BITS;
        this.TIMESTAMP_SHIFT = this.SEQUENCE_BITS + this.NODE_ID_BITS;

        this.nodeId = BigInt(getNodeIdFromIp()) & this.MAX_NODE_ID;
        this.logicalTime = 0n;
        this.sequence = 0n;
    }

    physicalTimeMillis() {
        return BigInt(Date.now());
    }

    async nextIdString() {
        const physicalTime = this.physicalTimeMillis();
        const currentLogicalTime = this.logicalTime;

        let newLogicalTime;

        if (physicalTime > currentLogicalTime) {
            newLogicalTime = physicalTime;
            this.logicalTime = newLogicalTime;
            this.sequence = 0n;
        } else {
            newLogicalTime = currentLogicalTime;
            this.sequence = (this.sequence + 1n) & this.MAX_SEQUENCE;
            if (this.sequence === 0n) {
                newLogicalTime = this.logicalTime + 1n;
                this.logicalTime = newLogicalTime;
            }
        }

        const id = ((newLogicalTime - this.EPOCH) << this.TIMESTAMP_SHIFT) |
                   (this.nodeId << this.NODE_ID_SHIFT) |
                   this.sequence;

        return id.toString();
    }
}

module.exports = HlcSnowflake;
