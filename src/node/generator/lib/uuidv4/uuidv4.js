const crypto = require('crypto');

class UuidV4Generator {
    async nextIdString() {
        return crypto.randomUUID();
    }
}

module.exports = UuidV4Generator;
