package uuidv4

import (
	"crypto/rand"
	"fmt"
)

type UuidV4Generator struct{}

func New() *UuidV4Generator {
	return &UuidV4Generator{}
}

func (g *UuidV4Generator) NextIdString() string {
	uuid := make([]byte, 16)
	_, err := rand.Read(uuid)
	if err != nil {
		fmt.Println("Error generating random bytes:", err)
		return ""
	}

	// Set version (4) and variant (RFC4122)
	uuid[6] = (uuid[6] & 0x0f) | 0x40
	uuid[8] = (uuid[8] & 0x3f) | 0x80

	return fmt.Sprintf("%08x-%04x-%04x-%04x-%012x",
		uuid[0:4],
		uuid[4:6],
		uuid[6:8],
		uuid[8:10],
		uuid[10:16])
}
