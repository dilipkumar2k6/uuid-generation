# Python Concepts in the UUID Generator

This document explains the key Python concepts used throughout the UUID generator implementation. It serves as a companion to `CPP.md`, `GOLANG.md`, `JAVA.md`, and `NODEJS.md` for developers familiarizing themselves with the Python version of the codebase.

## 1. Threads and Concurrency (`threading` module)

Python uses the `threading` module for concurrent execution. While the Global Interpreter Lock (GIL) prevents true parallel execution of Python bytecode on multiple cores, threads are still highly effective for I/O-bound tasks like network communication.

**Usage in this project:**
- **App Container:** We use `threading.Thread` to simulate multiple concurrent clients requesting UUIDs from the sidecar.
- **Generator Sidecar:** We spawn a new thread (`threading.Thread(target=handle_client, ...)`) for each incoming connection to handle requests concurrently without blocking the main server loop.

**Example:**
```python
import threading

def request_uuid(thread_id):
    # ... network request logic ...

t = threading.Thread(target=request_uuid, args=(1,))
t.daemon = True # Allows the program to exit even if this thread is running
t.start()
```

## 2. Synchronization (`threading.Lock`)

Because multiple threads might access and modify shared state (like the sequence counter or the last timestamp in the Snowflake algorithm), we must use locks to prevent race conditions.

**Usage in this project:**
We use `threading.Lock()` to create critical sections where only one thread can execute at a time. The `with self.lock:` statement ensures the lock is acquired before the block and released afterward, even if an exception occurs.

**Example:**
```python
import threading

class Snowflake:
    def __init__(self):
        self.lock = threading.Lock()
        self.sequence = 0

    def next_id_string(self):
        with self.lock:
            # Critical section: only one thread can be here at a time
            self.sequence += 1
            # ...
```

## 3. Network I/O (`socket` module)

Python's standard `socket` module provides low-level network communication capabilities.

**Usage in this project:**
- **Generator Sidecar:** Uses `socket.socket()` to create a TCP server, `bind()` to a port, `listen()` for connections, and `accept()` to receive them.
- **App Container:** Uses `socket.socket()` and `connect()` to establish a connection to the sidecar.

**Example (Server):**
```python
import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(('0.0.0.0', 8080))
server.listen(5)

while True:
    client_socket, addr = server.accept()
    # Handle connection...
```

## 4. Large Integers

Unlike C++ or Java, Python 3 automatically handles arbitrarily large integers. There is no need for specific 64-bit types (like `long long` in C++ or `long` in Java) or special classes (like `BigInt` in Node.js).

**Usage in this project:**
We perform bitwise operations and arithmetic directly on standard Python integers, and they automatically scale to accommodate the 64-bit values required for UUID generation.

**Example:**
```python
id = ((timestamp - self.EPOCH) << self.TIMESTAMP_SHIFT) | \
     (self.node_id << self.NODE_ID_SHIFT) | \
     self.sequence
```

## 5. HTTP Requests (`urllib.request`)

For interacting with external services over HTTP (like the Spanner emulators), we use Python's built-in `urllib.request` module to avoid external dependencies like `requests` in this simple setup.

**Usage in this project:**
Used in `SpannerGenerator` and `SpannerTrueTimeGenerator` to create sessions and execute SQL queries via the emulator's REST API.
