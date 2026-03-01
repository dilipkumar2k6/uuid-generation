const http = require('http');

class SpannerGenerator {
    constructor() {
        this.sessionName = null;
    }

    async init() {
        for (let i = 0; i < 30; i++) {
            try {
                await this.makeRequest('http://spanner:9020/', 'GET');
                break;
            } catch (err) {
                console.log("Waiting for Spanner emulator...");
                await new Promise(resolve => setTimeout(resolve, 2000));
            }
        }

        try {
            const dbName = "projects/test-project/instances/test-instance/databases/test-db";
            const response = await this.makeRequest(`http://spanner:9020/v1/${dbName}/sessions`, 'POST', {});
            const data = JSON.parse(response);
            if (data.name) {
                this.sessionName = data.name;
                console.log(`Successfully created Spanner session: ${this.sessionName}`);
            } else {
                console.error("Session name not found in response:", data);
                process.exit(1);
            }
        } catch (err) {
            console.error("Failed to initialize Spanner session:", err);
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
            console.error("Spanner session not initialized.");
            return "";
        }

        try {
            const url = `http://spanner:9020/v1/${this.sessionName}:executeSql`;
            const body = { sql: "SELECT GET_NEXT_SEQUENCE_VALUE(SEQUENCE global_id_seq)" };
            const response = await this.makeRequest(url, 'POST', body);
            const data = JSON.parse(response);

            if (data.rows && data.rows.length > 0 && data.rows[0].length > 0) {
                return data.rows[0][0];
            }
            console.error("Failed to extract sequence value from response:", data);
            return "";
        } catch (err) {
            console.error("Error executing SQL:", err);
            return "";
        }
    }
}

module.exports = SpannerGenerator;
