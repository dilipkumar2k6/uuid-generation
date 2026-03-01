const mysql = require('mysql2/promise');

class DualBufferGenerator {
    constructor() {
        this.pool = null;
        this.buffers = [
            { currentId: 0n, maxId: 0n, ready: false },
            { currentId: 0n, maxId: 0n, ready: false }
        ];
        this.activeIdx = 0;
        this.isFetching = false;
        this.mutex = Promise.resolve();
    }

    async init() {
        const config = {
            host: 'mysql-dual-buffer',
            port: 3306,
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
                console.log("Successfully connected to MySQL.");
                break;
            } catch (err) {
                console.log("Waiting for database connection...");
                await new Promise(resolve => setTimeout(resolve, 2000));
            }
        }

        if (!this.pool) {
            console.error("Database connection failed after retries.");
            process.exit(1);
        }

        await this.fetchNextBlock(0);
        if (!this.buffers[0].ready) {
            console.error("Failed to fetch initial ID segment from database");
            process.exit(1);
        }
    }

    async fetchNextBlock(bufferIdx) {
        if (this.isFetching) return;
        this.isFetching = true;

        let connection;
        try {
            connection = await this.pool.getConnection();
            await connection.beginTransaction();

            const [rows] = await connection.execute("SELECT max_id, step FROM id_generator WHERE biz_tag = 'user' FOR UPDATE");
            if (rows.length === 0) {
                throw new Error("No row found for biz_tag = 'user'");
            }

            const maxId = BigInt(rows[0].max_id);
            const step = BigInt(rows[0].step);
            const newMaxId = maxId + step;

            await connection.execute("UPDATE id_generator SET max_id = ? WHERE biz_tag = 'user'", [newMaxId.toString()]);
            await connection.commit();

            this.buffers[bufferIdx].currentId = maxId;
            this.buffers[bufferIdx].maxId = newMaxId;
            this.buffers[bufferIdx].ready = true;

            console.log(`Fetched new block for buffer ${bufferIdx}: [${maxId}, ${newMaxId})`);
        } catch (err) {
            console.error("Error fetching next block:", err);
            if (connection) {
                await connection.rollback();
            }
        } finally {
            if (connection) {
                connection.release();
            }
            this.isFetching = false;
        }
    }

    async nextIdString() {
        return new Promise((resolve, reject) => {
            this.mutex = this.mutex.then(async () => {
                let activeBuffer = this.buffers[this.activeIdx];

                if (activeBuffer.currentId >= activeBuffer.maxId) {
                    const standbyIdx = 1 - this.activeIdx;
                    let standbyBuffer = this.buffers[standbyIdx];

                    while (!standbyBuffer.ready) {
                        await new Promise(resolve => setTimeout(resolve, 10));
                    }

                    this.activeIdx = standbyIdx;
                    activeBuffer.ready = false;
                    activeBuffer = standbyBuffer;
                }

                const id = activeBuffer.currentId++;

                const threshold = activeBuffer.maxId - BigInt(Math.floor(Number(activeBuffer.maxId - id) * 0.9));
                if (activeBuffer.currentId === threshold) {
                    const standbyIdx = 1 - this.activeIdx;
                    this.fetchNextBlock(standbyIdx).catch(console.error);
                }

                resolve(id.toString());
            }).catch(reject);
        });
    }
}

module.exports = DualBufferGenerator;
