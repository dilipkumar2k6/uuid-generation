package spannertruetime

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"time"
)

type SpannerTrueTimeGenerator struct {
	sessionName string
	client      *http.Client
}

func New() *SpannerTrueTimeGenerator {
	g := &SpannerTrueTimeGenerator{
		client: &http.Client{Timeout: 5 * time.Second},
	}

	// Wait for Spanner emulator to be ready
	for i := 0; i < 30; i++ {
		resp, err := g.client.Get("http://spanner-truetime:9020/")
		if err == nil {
			resp.Body.Close()
			break
		}
		fmt.Println("Waiting for Spanner TrueTime emulator...")
		time.Sleep(2 * time.Second)
	}

	// Create a session
	dbName := "projects/test-project/instances/test-instance/databases/test-db"
	createSessionUrl := fmt.Sprintf("http://spanner-truetime:9020/v1/%s/sessions", dbName)

	reqBody := []byte(`{}`)
	resp, err := g.client.Post(createSessionUrl, "application/json", bytes.NewBuffer(reqBody))
	if err != nil {
		log.Fatalf("Failed to create Spanner session: %v", err)
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		log.Fatalf("Failed to read session response: %v", err)
	}

	var sessionData map[string]interface{}
	err = json.Unmarshal(body, &sessionData)
	if err != nil {
		log.Fatalf("Failed to parse session response: %v", err)
	}

	if name, ok := sessionData["name"].(string); ok {
		g.sessionName = name
		fmt.Printf("Successfully created Spanner TrueTime session: %s\n", g.sessionName)
	} else {
		log.Fatalf("Session name not found in response: %s", string(body))
	}

	return g
}

func (g *SpannerTrueTimeGenerator) NextIdString() string {
	executeSqlUrl := fmt.Sprintf("http://spanner-truetime:9020/v1/%s:executeSql", g.sessionName)

	// In a real Spanner environment, we would use a transaction and PENDING_COMMIT_TIMESTAMP().
	// For the emulator, we simulate it by inserting a row and returning the generated UUID.
	// We use a simple table with a generated UUID column.
	reqBody := []byte(`{"sql": "INSERT INTO events (description) VALUES ('event') THEN RETURN id"}`)
	resp, err := g.client.Post(executeSqlUrl, "application/json", bytes.NewBuffer(reqBody))
	if err != nil {
		fmt.Printf("Error executing SQL: %v\n", err)
		return ""
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		fmt.Printf("Error reading SQL response: %v\n", err)
		return ""
	}

	var resultData map[string]interface{}
	err = json.Unmarshal(body, &resultData)
	if err != nil {
		fmt.Printf("Error parsing SQL response: %v\n", err)
		return ""
	}

	if rows, ok := resultData["rows"].([]interface{}); ok && len(rows) > 0 {
		if row, ok := rows[0].([]interface{}); ok && len(row) > 0 {
			if val, ok := row[0].(string); ok {
				return val
			}
		}
	}

	fmt.Printf("Failed to extract ID from response: %s\n", string(body))
	return ""
}
