# Golang Concepts in the UUID Generator

This document explains the key Golang concepts used throughout the UUID generator implementation. It serves as a companion to `CPP.md` for developers familiarizing themselves with the Go version of the codebase.

## 1. Goroutines and Concurrency

Go's primary unit of concurrency is the **goroutine**. A goroutine is a lightweight thread managed by the Go runtime.

**Usage in this project:**
In the `app` container (`src/go/app/main.go`), we use goroutines to simulate multiple concurrent clients requesting UUIDs from the sidecar.

**Example:**
```go
func requestUUID(threadID int) {
    // ... logic to connect to sidecar and request UUID ...
}

func main() {
    const numThreads = 5
    var wg sync.WaitGroup

    for i := 1; i <= numThreads; i++ {
        wg.Add(1)
        go func(id int) {
            defer wg.Done()
            requestUUID(id)
        }(i) // Launch as a goroutine
    }

    wg.Wait() // Wait for all goroutines to finish
}
```

## 2. Channels

Channels are the pipes that connect concurrent goroutines. You can send values into channels from one goroutine and receive those values into another goroutine.

**Usage in this project:**
While the core ID generation relies heavily on atomic operations for performance, channels are used in specific algorithms like `DUAL_BUFFER` to signal the background thread to fetch the next block of IDs.

## 3. Atomic Operations (`sync/atomic`)

For high-performance, lock-free synchronization, Go provides the `sync/atomic` package. This is crucial for the Snowflake algorithm where multiple requests might arrive in the same millisecond.

**Usage in this project:**
We use `atomic.Uint64` (introduced in Go 1.19) or `atomic.AddUint64` / `atomic.LoadUint64` to manage the sequence counter and the last timestamp safely across concurrent requests without the overhead of mutexes.

**Example:**
```go
import "sync/atomic"

type Snowflake struct {
    sequence      atomic.Uint64
    lastTimestamp atomic.Uint64
    // ...
}

func (s *Snowflake) NextID() uint64 {
    // ...
    // Increment sequence atomically
    seq := (s.sequence.Add(1)) & maxSequence
    // ...
}
```

## 4. Mutexes (`sync.Mutex`)

When atomic operations are not sufficient (e.g., when updating multiple related variables that must be kept consistent, or when interacting with non-thread-safe resources), we use `sync.Mutex` or `sync.RWMutex`.

**Usage in this project:**
Mutexes are used in algorithms like `DUAL_BUFFER` to protect the buffer state (switching between active and standby buffers) and in `DB_AUTO_INC` to manage the connection pool or ensure sequential access to the database if needed.

## 5. Network I/O (`net` package)

Go's standard library provides robust networking capabilities in the `net` package.

**Usage in this project:**
The `generator` sidecar uses `net.Listen` to create a TCP server, and `net.Conn` to handle incoming connections from the `app` container. The `app` container uses `net.Dial` to connect to the sidecar.

**Example (Server):**
```go
listener, err := net.Listen("tcp", ":8080")
if err != nil {
    log.Fatal(err)
}
defer listener.Close()

for {
    conn, err := listener.Accept()
    if err != nil {
        log.Println("Accept error:", err)
        continue
    }
    go handleConnection(conn, generator)
}
```

## 6. Interfaces

Interfaces in Go provide a way to specify the behavior of an object: if something can do *this*, then it can be used *here*.

**Usage in this project:**
We define an `IdGenerator` interface that all specific algorithm implementations (Snowflake, Sonyflake, etc.) must satisfy. This allows the main server loop to be agnostic to the underlying algorithm.

**Example:**
```go
type IdGenerator interface {
    NextIdString() string
}

// Snowflake implements IdGenerator
type Snowflake struct { // ... }
func (s *Snowflake) NextIdString() string { // ... }

// Usage
var gen IdGenerator = NewSnowflake()
uuidStr := gen.NextIdString()
```
