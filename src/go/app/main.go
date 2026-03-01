package main

import (
	"fmt"
	"net"
	"sync"
	"time"
)

func requestUUID(threadID int, wg *sync.WaitGroup) {
	defer wg.Done()
	// Continuously request UUIDs from the Snowflake sidecar
	for {
		// 1. Attempt to connect to the sidecar (localhost:8080 for sidecar IPC)
		conn, err := net.Dial("tcp", "127.0.0.1:8080")
		if err != nil {
			fmt.Printf("[Thread %d] Connection Failed. Retrying...\n", threadID)
			time.Sleep(1 * time.Second)
			continue
		}

		// 2. Read the UUID string from the socket
		buffer := make([]byte, 128)
		n, err := conn.Read(buffer)
		if err != nil {
			fmt.Printf("[Thread %d] Failed to read UUID\n", threadID)
		} else if n > 0 {
			fmt.Printf("[Thread %d] Received UUID: %s\n", threadID, string(buffer[:n]))
		}

		// 3. Close the socket and wait before the next request
		conn.Close()
		time.Sleep(500 * time.Millisecond) // Request every 500ms
	}
}

func main() {
	fmt.Println("App container starting with 5 concurrent threads...")

	const numThreads = 5
	var wg sync.WaitGroup

	// Spawn multiple goroutines to simulate concurrent requests
	for i := 1; i <= numThreads; i++ {
		wg.Add(1)
		go requestUUID(i, &wg)
	}

	// Wait for all goroutines (will run indefinitely)
	wg.Wait()
}
