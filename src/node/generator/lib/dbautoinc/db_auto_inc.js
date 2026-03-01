const mysql = require('mysql2/promise');

class DbAutoIncGenerator {
    constructor() {
        this.pool = null;
    }

    async init() {
        const config = {
            host: 'proxysql',
            port: 6033,
            user: 'root',
            password: 'rootpassword',
            database: 'uuid_db',
            waitForConnections: true,
            connectionLimit: 10,
            queueLimit: 0
        };

        for (let i = 0; i < 30; i++) {
            try {
                this.pool = mysql.createPool(config);
                const connection = await this.pool.getConnection();
                connection.release();
                console.log("Successfully connected to ProxySQL.");
                return;
            } catch (err) {
                console.log("Waiting for database connection...");
                await new Promise(resolve => setTimeout(resolve, 2000));
            }
        }

        console.error("Database connection failed after retries.");
        process.exit(1);
    }

    async nextIdString() {
        if (!this.pool) {
            console.error("Database pool not initialized.");
            return "";
        }

        try {
            const [result] = await this.pool.execute("REPLACE INTO tickets64 (stub) VALUES ('a')");
            return result.insertId.toString();
        } catch (err) {
            console.error("Error executing query:", err);
            return "";
        }
    }
}

module.exports = DbAutoIncGenerator;
