package dualbuffer

import (
	"database/sql"
	"fmt"
	"log"
	"strconv"
	"sync"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

type IdBuffer struct {
	currentId uint64
	maxId     uint64
	ready     bool
}

type DualBufferGenerator struct {
	db          *sql.DB
	buffers     [2]IdBuffer
	activeIdx   int
	mutex       sync.Mutex
	fetchSignal chan struct{}
}

func New() *DualBufferGenerator {
	dsn := "root:rootpassword@tcp(mysql-dual-buffer:3306)/uuid_db"
	db, err := sql.Open("mysql", dsn)
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}

	for i := 0; i < 30; i++ {
		err = db.Ping()
		if err == nil {
			break
		}
		fmt.Println("Waiting for database connection...")
		time.Sleep(2 * time.Second)
	}

	if err != nil {
		log.Fatalf("Database connection failed after retries: %v", err)
	}

	fmt.Println("Successfully connected to MySQL.")

	g := &DualBufferGenerator{
		db:          db,
		activeIdx:   0,
		fetchSignal: make(chan struct{}, 1),
	}

	// Fetch initial block for the first buffer synchronously
	g.fetchNextBlock(0)
	if !g.buffers[0].ready {
		log.Fatal("Failed to fetch initial ID segment from database")
	}

	// Start background thread to fetch subsequent blocks
	go g.backgroundFetcher()

	return g
}

func (g *DualBufferGenerator) fetchNextBlock(bufferIdx int) {
	tx, err := g.db.Begin()
	if err != nil {
		fmt.Printf("Error beginning transaction: %v\n", err)
		return
	}
	defer tx.Rollback()

	// 1. Get the current max_id and step for the 'user' tag
	var maxId, step uint64
	err = tx.QueryRow("SELECT max_id, step FROM id_generator WHERE biz_tag = 'user' FOR UPDATE").Scan(&maxId, &step)
	if err != nil {
		fmt.Printf("Error querying id_generator: %v\n", err)
		return
	}

	// 2. Calculate the new max_id for this block
	newMaxId := maxId + step

	// 3. Update the database with the new max_id
	_, err = tx.Exec("UPDATE id_generator SET max_id = ? WHERE biz_tag = 'user'", newMaxId)
	if err != nil {
		fmt.Printf("Error updating id_generator: %v\n", err)
		return
	}

	err = tx.Commit()
	if err != nil {
		fmt.Printf("Error committing transaction: %v\n", err)
		return
	}

	// 4. Update the target buffer
	g.mutex.Lock()
	g.buffers[bufferIdx].currentId = maxId
	g.buffers[bufferIdx].maxId = newMaxId
	g.buffers[bufferIdx].ready = true
	g.mutex.Unlock()

	fmt.Printf("Fetched new block for buffer %d: [%d, %d)\n", bufferIdx, maxId, newMaxId)
}

func (g *DualBufferGenerator) backgroundFetcher() {
	for {
		<-g.fetchSignal // Wait for a signal to fetch
		standbyIdx := 1 - g.activeIdx
		g.fetchNextBlock(standbyIdx)
	}
}

func (g *DualBufferGenerator) NextIdString() string {
	g.mutex.Lock()
	defer g.mutex.Unlock()

	activeBuffer := &g.buffers[g.activeIdx]

	// If the active buffer is exhausted, we need to switch
	if activeBuffer.currentId >= activeBuffer.maxId {
		standbyIdx := 1 - g.activeIdx
		standbyBuffer := &g.buffers[standbyIdx]

		// Wait for the standby buffer to be ready if it isn't already
		for !standbyBuffer.ready {
			g.mutex.Unlock()
			time.Sleep(10 * time.Millisecond) // Spin wait briefly
			g.mutex.Lock()
		}

		// Switch buffers
		g.activeIdx = standbyIdx
		activeBuffer.ready = false // Mark old active as not ready
		activeBuffer = standbyBuffer
	}

	// Generate ID from the active buffer
	id := activeBuffer.currentId
	activeBuffer.currentId++

	// If we've consumed 10% of the active buffer, signal the background thread
	// to start fetching the next block into the standby buffer
	threshold := activeBuffer.maxId - uint64(float64(activeBuffer.maxId-id)*0.9)
	if activeBuffer.currentId == threshold {
		select {
		case g.fetchSignal <- struct{}{}:
			// Signal sent
		default:
			// Signal already pending
		}
	}

	return strconv.FormatUint(id, 10)
}
