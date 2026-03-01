package uuidv7

import (
	"crypto/rand"
	"fmt"
	"time"
)

type UuidV7Generator struct{}

func New() *UuidV7Generator {
	return &UuidV7Generator{}
}

func (g *UuidV7Generator) NextIdString() string {
	uuid := make([]byte, 16)

	// 1. Get current Unix timestamp in milliseconds (48 bits)
	timestampMs := uint64(time.Now().UnixMilli())

	// 2. Fill the first 6 bytes with the timestamp
	uuid[0] = byte((timestampMs >> 40) & 0xFF)
	uuid[1] = byte((timestampMs >> 32) & 0xFF)
	uuid[2] = byte((timestampMs >> 24) & 0xFF)
	uuid[3] = byte((timestampMs >> 16) & 0xFF)
	uuid[4] = byte((timestampMs >> 8) & 0xFF)
	uuid[5] = byte(timestampMs & 0xFF)

	// 3. Fill the remaining 10 bytes with random data
	_, err := rand.Read(uuid[6:])
	if err != nil {
		fmt.Println("Error generating random bytes:", err)
		return ""
	}

	// 4. Set version (7) and variant (RFC4122)
	uuid[6] = (uuid[6] & 0x0F) | 0x70 // Version 7
	uuid[8] = (uuid[8] & 0x3F) | 0x80 // Variant 10

	return fmt.Sprintf("%08x-%04x-%04x-%04x-%012x",
		uuid[0:4],
		uuid[4:6],
		uuid[6:8],
		uuid[8:10],
		uuid[10:16])
}
