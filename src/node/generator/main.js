const net = require('net');

const DbAutoIncGenerator = require('./lib/dbautoinc/db_auto_inc');
const DualBufferGenerator = require('./lib/dualbuffer/dual_buffer');
const EtcdSnowflake = require('./lib/etcdsnowflake/etcd_snowflake');
const HlcSnowflake = require('./lib/hlcsnowflake/hlc_snowflake');
const InstaSnowflake = require('./lib/instasnowflake/insta_snowflake');
const Snowflake = require('./lib/snowflake/snowflake');
const Sonyflake = require('./lib/sonyflake/sonyflake');
const SpannerGenerator = require('./lib/spanner/spanner');
const SpannerTrueTimeGenerator = require('./lib/spannertruetime/spanner_truetime');
const UuidV4Generator = require('./lib/uuidv4/uuidv4');
const UuidV7Generator = require('./lib/uuidv7/uuidv7');

async function main() {
    // ---------------------------------------------------------
    // 1. Determine Generator Type
    // ---------------------------------------------------------
    const genType = process.env.GENERATOR_TYPE || "SNOWFLAKE";
    let generator;

    switch (genType) {
        case "HLC_SNOWFLAKE":
            console.log("Initializing HLC Snowflake generator...");
            generator = new HlcSnowflake();
            break;
        case "INSTA_SNOWFLAKE":
            console.log("Initializing Instagram Snowflake generator...");
            generator = new InstaSnowflake();
            break;
        case "SONYFLAKE":
            console.log("Initializing Sonyflake generator...");
            generator = new Sonyflake();
            break;
        case "UUIDV4":
            console.log("Initializing UUID Version 4 generator...");
            generator = new UuidV4Generator();
            break;
        case "UUIDV7":
            console.log("Initializing UUID Version 7 generator...");
            generator = new UuidV7Generator();
            break;
        case "DB_AUTO_INC":
            console.log("Initializing Database Auto-Increment generator...");
            generator = new DbAutoIncGenerator();
            await generator.init();
            break;
        case "DUAL_BUFFER":
            console.log("Initializing Dual Buffer generator...");
            generator = new DualBufferGenerator();
            await generator.init();
            break;
        case "ETCD_SNOWFLAKE":
            console.log("Initializing Etcd-Coordinated Snowflake generator...");
            generator = new EtcdSnowflake();
            break;
        case "SPANNER":
            console.log("Initializing Spanner Sequence generator...");
            generator = new SpannerGenerator();
            await generator.init();
            break;
        case "SPANNER_TRUETIME":
            console.log("Initializing Spanner TrueTime generator...");
            generator = new SpannerTrueTimeGenerator();
            await generator.init();
            break;
        default:
            console.log("Initializing Standard Snowflake generator...");
            generator = new Snowflake();
            break;
    }

    // ---------------------------------------------------------
    // 2. Setup TCP Server Socket
    // ---------------------------------------------------------
    const server = net.createServer(async (socket) => {
        try {
            const uuidStr = await generator.nextIdString();
            socket.write(uuidStr);
            socket.end(); // Close the connection immediately after sending
        } catch (err) {
            console.error("Error generating ID:", err);
            socket.end();
        }
    });

    server.on('error', (err) => {
        console.error("Server error:", err);
    });

    server.listen(8080, () => {
        console.log("Sidecar listening on port 8080...");
    });
}

main().catch(console.error);
