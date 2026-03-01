package spanner

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"time"
)

type SpannerGenerator struct {
	sessionName string
	client      *http.Client
}

func New() *SpannerGenerator {
	g := &SpannerGenerator{
		client: &http.Client{Timeout: 5 * time.Second},
	}

	// 1. Wait for Spanner emulator to be ready
	for i := 0; i < 30; i++ {
		resp, err := g.client.Get("http://spanner:9020/")
		if err == nil {
			resp.Body.Close()
			break
		}
		fmt.Println("Waiting for Spanner emulator...")
		time.Sleep(2 * time.Second)
	}

	// 2. Create a session
	dbName := "projects/test-project/instances/test-instance/databases/test-db"
	createSessionUrl := fmt.Sprintf("http://spanner:9020/v1/%s/sessions", dbName)

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
		fmt.Printf("Successfully created Spanner session: %s\n", g.sessionName)
	} else {
		log.Fatalf("Session name not found in response: %s", string(body))
	}

	return g
}

func (g *SpannerGenerator) NextIdString() string {
	executeSqlUrl := fmt.Sprintf("http://spanner:9020/v1/%s:executeSql", g.sessionName)

	// Use the bit_reversed_positive sequence
	reqBody := []byte(`{"sql": "SELECT GET_NEXT_SEQUENCE_VALUE(SEQUENCE global_id_seq)"}`)
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

	// The response format is complex, we need to extract the value
	// It looks like: {"metadata": {...}, "rows": [["12345"]]}
	if rows, ok := resultData["rows"].([]interface{}); ok && len(rows) > 0 {
		if row, ok := rows[0].([]interface{}); ok && len(row) > 0 {
			if val, ok := row[0].(string); ok {
				// The value is returned as a string representing the 64-bit integer
				return val
			}
		}
	}

	fmt.Printf("Failed to extract sequence value from response: %s\n", string(body))
	return ""
}
