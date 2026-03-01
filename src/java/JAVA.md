# Java Concepts in the UUID Generator

This document explains the key Java concepts used throughout the UUID generator implementation. It serves as a companion to `CPP.md` and `GOLANG.md` for developers familiarizing themselves with the Java version of the codebase.

## 1. Threads and Concurrency

Java's primary unit of concurrency is the **Thread**.

**Usage in this project:**
In the `app` container (`src/java/app/App.java`), we use `Thread` objects to simulate multiple concurrent clients requesting UUIDs from the sidecar.

**Example:**
```java
int numThreads = 5;
for (int i = 1; i <= numThreads; i++) {
    final int threadId = i;
    new Thread(() -> requestUUID(threadId)).start();
}
```

## 2. Atomic Operations (`java.util.concurrent.atomic`)

For high-performance, lock-free synchronization, Java provides the `java.util.concurrent.atomic` package. This is crucial for the Snowflake algorithm where multiple requests might arrive in the same millisecond.

**Usage in this project:**
We use `AtomicLong` to manage the sequence counter and the last timestamp safely across concurrent requests without the overhead of synchronized blocks.

**Example:**
```java
import java.util.concurrent.atomic.AtomicLong;

public class Snowflake {
    private final AtomicLong sequence = new AtomicLong(0L);
    // ...

    public long nextId() {
        // ...
        // Increment sequence atomically
        long seq = (sequence.incrementAndGet()) & MAX_SEQUENCE;
        // ...
    }
}
```

## 3. Locks (`java.util.concurrent.locks.ReentrantLock`)

When atomic operations are not sufficient (e.g., when updating multiple related variables that must be kept consistent), we use `ReentrantLock`.

**Usage in this project:**
Locks are used in algorithms like `DUAL_BUFFER` to protect the buffer state (switching between active and standby buffers).

## 4. Network I/O (`java.net` package)

Java's standard library provides robust networking capabilities in the `java.net` package.

**Usage in this project:**
The `generator` sidecar uses `ServerSocket` to create a TCP server, and `Socket` to handle incoming connections from the `app` container. The `app` container uses `Socket` to connect to the sidecar.

**Example (Server):**
```java
try (ServerSocket serverSocket = new ServerSocket(8080)) {
    while (true) {
        Socket clientSocket = serverSocket.accept();
        // Handle connection...
    }
}
```

## 5. Interfaces

Interfaces in Java provide a way to specify the behavior of an object.

**Usage in this project:**
We define an `IdGenerator` interface that all specific algorithm implementations (Snowflake, Sonyflake, etc.) must implement. This allows the main server loop to be agnostic to the underlying algorithm.

**Example:**
```java
public interface IdGenerator {
    String nextIdString();
}

// Snowflake implements IdGenerator
public class Snowflake implements IdGenerator {
    @Override
    public String nextIdString() { // ... }
}
```
