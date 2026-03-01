package main

import (
	"fmt"
	"log"
	"net"
	"os"

	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/dbautoinc"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/dualbuffer"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/etcdsnowflake"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/hlcsnowflake"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/instasnowflake"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/snowflake"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/sonyflake"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/spanner"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/spannertruetime"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/uuidv4"
	"github.com/dilipkumardk/uuid-generator/src/go/generator/lib/uuidv7"
)

func main() {
	// ---------------------------------------------------------
	// 1. Determine Generator Type
	// ---------------------------------------------------------
	genType := os.Getenv("GENERATOR_TYPE")
	if genType == "" {
		genType = "SNOWFLAKE"
	}

	var generator lib.IdGenerator

	switch genType {
	case "HLC_SNOWFLAKE":
		fmt.Println("Initializing HLC Snowflake generator...")
		generator = hlcsnowflake.New()
	case "INSTA_SNOWFLAKE":
		fmt.Println("Initializing Instagram Snowflake generator...")
		generator = instasnowflake.New()
	case "SONYFLAKE":
		fmt.Println("Initializing Sonyflake generator...")
		generator = sonyflake.New()
	case "UUIDV4":
		fmt.Println("Initializing UUID Version 4 generator...")
		generator = uuidv4.New()
	case "UUIDV7":
		fmt.Println("Initializing UUID Version 7 generator...")
		generator = uuidv7.New()
	case "DB_AUTO_INC":
		fmt.Println("Initializing Database Auto-Increment generator...")
		generator = dbautoinc.New()
	case "DUAL_BUFFER":
		fmt.Println("Initializing Dual Buffer generator...")
		generator = dualbuffer.New()
	case "ETCD_SNOWFLAKE":
		fmt.Println("Initializing Etcd-Coordinated Snowflake generator...")
		generator = etcdsnowflake.New()
	case "SPANNER":
		fmt.Println("Initializing Spanner Sequence generator...")
		generator = spanner.New()
	case "SPANNER_TRUETIME":
		fmt.Println("Initializing Spanner TrueTime generator...")
		generator = spannertruetime.New()
	default:
		fmt.Println("Initializing Standard Snowflake generator...")
		generator = snowflake.New()
	}

	// ---------------------------------------------------------
	// 2. Setup TCP Server Socket
	// ---------------------------------------------------------
	listener, err := net.Listen("tcp", ":8080")
	if err != nil {
		log.Fatalf("Listen failed: %v", err)
	}
	defer listener.Close()

	fmt.Println("Sidecar listening on port 8080...")

	// ---------------------------------------------------------
	// 3. Main Server Loop
	// ---------------------------------------------------------
	for {
		// Accept an incoming connection
		conn, err := listener.Accept()
		if err != nil {
			log.Printf("Accept failed: %v", err)
			continue
		}

		// Generate a new UUID string and send it to the connected client
		uuidStr := generator.NextIdString()
		conn.Write([]byte(uuidStr))

		// Close the connection immediately after sending (stateless IPC)
		conn.Close()
	}
}
