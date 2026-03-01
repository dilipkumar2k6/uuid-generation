const { getNodeIdFromIp } = require('../network_util');

class Sonyflake {
    constructor() {
        this.EPOCH = 1767225600000n; // Jan 1, 2026
        this.MACHINE_ID_BITS = 16n;
        this.SEQUENCE_BITS = 8n;
        this.MAX_MACHINE_ID = (1n << this.MACHINE_ID_BITS) - 1n;
        this.MAX_SEQUENCE = (1n << this.SEQUENCE_BITS) - 1n;
        this.MACHINE_ID_SHIFT = this.SEQUENCE_BITS;
        this.TIMESTAMP_SHIFT = this.SEQUENCE_BITS + this.MACHINE_ID_BITS;

        this.machineId = BigInt(getNodeIdFromIp()) & this.MAX_MACHINE_ID;
        this.sequence = 0n;
        this.lastTimestamp = -1n;
    }

    currentTime10ms() {
        return BigInt(Math.floor(Date.now() / 10));
    }

    waitForNext10ms(lastTs) {
        let timestamp = this.currentTime10ms();
        while (timestamp <= lastTs) {
            timestamp = this.currentTime10ms();
        }
        return timestamp;
    }

    async nextIdString() {
        let timestamp = this.currentTime10ms();

        if (timestamp < this.lastTimestamp) {
            console.error("Clock moved backwards. Refusing to generate id.");
            return "0";
        }

        if (timestamp === this.lastTimestamp) {
            this.sequence = (this.sequence + 1n) & this.MAX_SEQUENCE;
            if (this.sequence === 0n) {
                timestamp = this.waitForNext10ms(this.lastTimestamp);
            }
        } else {
            this.sequence = 0n;
        }

        this.lastTimestamp = timestamp;

        const id = ((timestamp - (this.EPOCH / 10n)) << this.TIMESTAMP_SHIFT) |
                   (this.machineId << this.MACHINE_ID_SHIFT) |
                   this.sequence;

        return id.toString();
    }
}

module.exports = Sonyflake;
