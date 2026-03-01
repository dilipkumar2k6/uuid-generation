const http = require('http');

class SpannerTrueTimeGenerator {
    constructor() {
        this.sessionName = null;
    }

    async init() {
        for (let i = 0; i < 30; i++) {
            try {
                await this.makeRequest('http://spanner:9020/', 'GET');
                break;
            } catch (err) {
                console.log("Waiting for Spanner TrueTime emulator...");
                await new Promise(resolve => setTimeout(resolve, 2000));
            }
        }

        try {
            const dbName = "projects/test-project/instances/test-instance/databases/test-db";
            const response = await this.makeRequest(`http://spanner:9020/v1/${dbName}/sessions`, 'POST', {});
            const data = JSON.parse(response);
            if (data.name) {
                this.sessionName = data.name;
                console.log(`Successfully created Spanner TrueTime session: ${this.sessionName}`);
            } else {
                console.error("Session name not found in response:", data);
                process.exit(1);
            }
        } catch (err) {
            console.error("Failed to initialize Spanner TrueTime session:", err);
            process.exit(1);
        }
    }

    makeRequest(url, method, body = null) {
        return new Promise((resolve, reject) => {
            const { hostname, port, pathname } = new URL(url);
            const options = {
                hostname,
                port: port || 80,
                path: pathname,
                method,
                headers: {
                    'Content-Type': 'application/json'
                }
            };

            const req = http.request(options, (res) => {
                let data = '';
                res.on('data', (chunk) => {
                    data += chunk;
                });
                res.on('end', () => {
                    if (res.statusCode >= 200 && res.statusCode < 300) {
                        resolve(data);
                    } else if (res.statusCode === 404 && method === 'GET') {
                        resolve(data); // Emulator returns 404 for root GET but it means it's up
                    } else {
                        reject(new Error(`Request failed with status code ${res.statusCode}: ${data}`));
                    }
                });
            });

            req.on('error', (err) => {
                reject(err);
            });

            if (body) {
                req.write(JSON.stringify(body));
            }
            req.end();
        });
    }

    async nextIdString() {
        if (!this.sessionName) {
            console.error("Spanner TrueTime session not initialized.");
            return "";
        }

        try {
            // 1. Begin Transaction
            const beginTxUrl = `http://spanner:9020/v1/${this.sessionName}:beginTransaction`;
            const beginTxBody = {
                options: {
                    readWrite: {}
                }
            };
            const beginTxResponse = await this.makeRequest(beginTxUrl, 'POST', beginTxBody);
            const txData = JSON.parse(beginTxResponse);
            const transactionId = txData.id;

            if (!transactionId) {
                console.error("Failed to begin transaction:", txData);
                return "";
            }

            // 2. Execute SQL
            const url = `http://spanner:9020/v1/${this.sessionName}:executeSql`;
            const body = { 
                sql: "INSERT INTO events (description) VALUES ('event') THEN RETURN id",
                transaction: {
                    id: transactionId
                }
            };
            const response = await this.makeRequest(url, 'POST', body);
            const data = JSON.parse(response);

            let generatedId = "";
            if (data.rows && data.rows.length > 0 && data.rows[0].length > 0) {
                generatedId = data.rows[0][0];
            } else {
                console.error("Failed to extract ID from response:", data);
            }

            // 3. Commit Transaction
            const commitUrl = `http://spanner:9020/v1/${this.sessionName}:commit`;
            const commitBody = {
                transactionId: transactionId,
                mutations: []
            };
            await this.makeRequest(commitUrl, 'POST', commitBody);

            return generatedId;
        } catch (err) {
            console.error("Error executing SQL:", err);
            return "";
        }
    }
}

module.exports = SpannerTrueTimeGenerator;
