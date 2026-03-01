const net = require('net');

console.log("App container starting with 5 concurrent connections...");

const numConnections = 5;

for (let i = 1; i <= numConnections; i++) {
    requestUUID(i);
}

function requestUUID(connectionId) {
    const connect = () => {
        const client = new net.Socket();

        client.connect(8080, '127.0.0.1', () => {
            // Connected, wait for data
        });

        client.on('data', (data) => {
            console.log(`[Connection ${connectionId}] Received UUID: ${data.toString()}`);
            client.destroy(); // kill client after server's response
        });

        client.on('close', () => {
            setTimeout(connect, 500); // Reconnect after 500ms
        });

        client.on('error', (err) => {
            console.error(`[Connection ${connectionId}] Connection Failed. Retrying...`);
            client.destroy();
        });
    };

    connect();
}
