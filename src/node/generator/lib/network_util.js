const os = require('os');

function getNodeIdFromIp() {
    const interfaces = os.networkInterfaces();
    for (const name of Object.keys(interfaces)) {
        for (const iface of interfaces[name]) {
            if (iface.family === 'IPv4' && !iface.internal) {
                const ipParts = iface.address.split('.');
                if (ipParts.length === 4) {
                    const part3 = parseInt(ipParts[2], 10);
                    const part4 = parseInt(ipParts[3], 10);
                    // Take the last 2 bits of part3 and all 8 bits of part4
                    return ((part3 & 0x03) << 8) | part4;
                }
            }
        }
    }
    return 0;
}

module.exports = { getNodeIdFromIp };
