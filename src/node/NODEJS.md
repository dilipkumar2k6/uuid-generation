# Node.js Concepts in the UUID Generator

This document explains the key Node.js concepts used throughout the UUID generator implementation. It serves as a companion to `CPP.md`, `GOLANG.md`, and `JAVA.md` for developers familiarizing themselves with the Node.js version of the codebase.

## 1. Asynchronous Programming (Promises and async/await)

Node.js is inherently single-threaded and uses an event-driven, non-blocking I/O model. This makes asynchronous programming crucial.

**Usage in this project:**
We use `async`/`await` extensively for network requests (e.g., to Spanner) and database queries (e.g., MySQL). The `nextIdString()` method in all generators is `async` to allow for potential asynchronous operations (like fetching a new block in `DualBuffer`).

**Example:**
```javascript
async function main() {
    // ...
    const uuidStr = await generator.nextIdString();
    socket.write(uuidStr);
    // ...
}
```

## 2. BigInt for 64-bit Integers

JavaScript's standard `Number` type is a double-precision float, which cannot safely represent 64-bit integers without losing precision. For UUID generation (like Snowflake), precise 64-bit integer math is required.

**Usage in this project:**
We use the `BigInt` primitive (denoted by the `n` suffix, e.g., `0n`, `1767225600000n`) for all bitwise operations and timestamp calculations in the Snowflake-based algorithms.

**Example:**
```javascript
const id = ((timestamp - this.EPOCH) << this.TIMESTAMP_SHIFT) |
           (this.nodeId << this.NODE_ID_SHIFT) |
           this.sequence;
return id.toString();
```

## 3. Network I/O (`net` module)

Node.js provides the `net` module for creating TCP servers and clients.

**Usage in this project:**
The `generator` sidecar uses `net.createServer()` to listen for incoming connections. The `app` container uses `new net.Socket()` to connect to the sidecar.

**Example (Server):**
```javascript
const net = require('net');

const server = net.createServer(async (socket) => {
    const uuidStr = await generator.nextIdString();
    socket.write(uuidStr);
    socket.end();
});

server.listen(8080);
```

## 4. Concurrency Control (Mutexes)

While Node.js is single-threaded, asynchronous operations can interleave. When multiple requests need to access shared state that involves asynchronous steps (like fetching a new block from the database in `DualBuffer`), we need concurrency control.

**Usage in this project:**
In `DualBufferGenerator`, we implement a simple Mutex using Promises to ensure that only one request attempts to switch buffers or trigger a fetch at a time, preventing race conditions.

**Example:**
```javascript
class DualBufferGenerator {
    constructor() {
        this.mutex = Promise.resolve();
    }

    async nextIdString() {
        return new Promise((resolve, reject) => {
            this.mutex = this.mutex.then(async () => {
                // Critical section
                // ...
                resolve(id.toString());
            }).catch(reject);
        });
    }
}
```

## 5. Classes and Modules

Node.js uses CommonJS modules (`require` and `module.exports`) for organizing code.

**Usage in this project:**
Each generator algorithm is encapsulated in an ES6 class and exported as a module. The `main.js` file requires these modules and instantiates the appropriate class based on the environment variable.
