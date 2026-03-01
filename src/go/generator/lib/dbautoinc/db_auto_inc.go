package dbautoinc

import (
	"database/sql"
	"fmt"
	"log"
	"strconv"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

type DbAutoIncGenerator struct {
	db *sql.DB
}

func New() *DbAutoIncGenerator {
	// Connect to ProxySQL (which routes to the MySQL masters)
	dsn := "root:rootpassword@tcp(proxysql:6033)/uuid_db"
	db, err := sql.Open("mysql", dsn)
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}

	// Wait for database to be ready
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

	fmt.Println("Successfully connected to ProxySQL.")
	return &DbAutoIncGenerator{db: db}
}

func (g *DbAutoIncGenerator) NextIdString() string {
	// REPLACE INTO ensures we update the existing row for 'stub'='a'
	// and trigger the auto-increment mechanism.
	query := "REPLACE INTO tickets64 (stub) VALUES ('a')"
	result, err := g.db.Exec(query)
	if err != nil {
		fmt.Printf("Error executing query: %v\n", err)
		return ""
	}

	id, err := result.LastInsertId()
	if err != nil {
		fmt.Printf("Error getting last insert ID: %v\n", err)
		return ""
	}

	return strconv.FormatInt(id, 10)
}
