package com.uuidgenerator.lib.spannertruetime;

import com.uuidgenerator.lib.IdGenerator;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

public class SpannerTrueTimeGenerator implements IdGenerator {
    private String sessionName;

    public SpannerTrueTimeGenerator() {
        try {
            // Wait for Spanner emulator to be ready
            for (int i = 0; i < 30; i++) {
                try {
                    URL url = new URL("http://spanner:9020/");
                    HttpURLConnection con = (HttpURLConnection) url.openConnection();
                    con.setRequestMethod("GET");
                    int responseCode = con.getResponseCode();
                    if (responseCode == 200 || responseCode == 404) {
                        break;
                    }
                } catch (Exception e) {
                    System.out.println("Waiting for Spanner TrueTime emulator...");
                    Thread.sleep(2000);
                }
            }

            // Create a session
            String dbName = "projects/test-project/instances/test-instance/databases/test-db";
            URL url = new URL("http://spanner:9020/v1/" + dbName + "/sessions");
            HttpURLConnection con = (HttpURLConnection) url.openConnection();
            con.setRequestMethod("POST");
            con.setRequestProperty("Content-Type", "application/json");
            con.setDoOutput(true);

            String jsonInputString = "{}";
            try (OutputStream os = con.getOutputStream()) {
                byte[] input = jsonInputString.getBytes(StandardCharsets.UTF_8);
                os.write(input, 0, input.length);
            }

            try (BufferedReader br = new BufferedReader(new InputStreamReader(con.getInputStream(), StandardCharsets.UTF_8))) {
                StringBuilder response = new StringBuilder();
                String responseLine;
                while ((responseLine = br.readLine()) != null) {
                    response.append(responseLine.trim());
                }
                
                // Simple JSON parsing to extract session name
                String json = response.toString();
                int nameIndex = json.indexOf("\"name\":");
                if (nameIndex != -1) {
                    int startQuote = json.indexOf("\"", nameIndex + 7);
                    int endQuote = json.indexOf("\"", startQuote + 1);
                    sessionName = json.substring(startQuote + 1, endQuote);
                    System.out.println("Successfully created Spanner TrueTime session: " + sessionName);
                } else {
                    System.err.println("Session name not found in response: " + json);
                    System.exit(1);
                }
            }
        } catch (Exception e) {
            System.err.println("Failed to initialize Spanner TrueTime session: " + e.getMessage());
            System.exit(1);
        }
    }

    @Override
    public String nextIdString() {
        try {
            // 1. Begin Transaction
            URL beginTxUrl = new URL("http://spanner:9020/v1/" + sessionName + ":beginTransaction");
            HttpURLConnection beginTxCon = (HttpURLConnection) beginTxUrl.openConnection();
            beginTxCon.setRequestMethod("POST");
            beginTxCon.setRequestProperty("Content-Type", "application/json");
            beginTxCon.setDoOutput(true);

            String beginTxJson = "{\"options\": {\"readWrite\": {}}}";
            try (OutputStream os = beginTxCon.getOutputStream()) {
                byte[] input = beginTxJson.getBytes(StandardCharsets.UTF_8);
                os.write(input, 0, input.length);
            }

            String transactionId = "";
            try (BufferedReader br = new BufferedReader(new InputStreamReader(beginTxCon.getInputStream(), StandardCharsets.UTF_8))) {
                StringBuilder response = new StringBuilder();
                String responseLine;
                while ((responseLine = br.readLine()) != null) {
                    response.append(responseLine.trim());
                }
                String json = response.toString();
                int idIndex = json.indexOf("\"id\":");
                if (idIndex != -1) {
                    int startQuote = json.indexOf("\"", idIndex + 5);
                    int endQuote = json.indexOf("\"", startQuote + 1);
                    transactionId = json.substring(startQuote + 1, endQuote);
                }
            }

            if (transactionId.isEmpty()) {
                System.err.println("Failed to begin transaction");
                return "";
            }

            // 2. Execute SQL
            URL url = new URL("http://spanner:9020/v1/" + sessionName + ":executeSql");
            HttpURLConnection con = (HttpURLConnection) url.openConnection();
            con.setRequestMethod("POST");
            con.setRequestProperty("Content-Type", "application/json");
            con.setDoOutput(true);

            String jsonInputString = "{\"sql\": \"INSERT INTO events (description) VALUES ('event') THEN RETURN id\", \"transaction\": {\"id\": \"" + transactionId + "\"}}";
            try (OutputStream os = con.getOutputStream()) {
                byte[] input = jsonInputString.getBytes(StandardCharsets.UTF_8);
                os.write(input, 0, input.length);
            }

            String generatedId = "";
            try (BufferedReader br = new BufferedReader(new InputStreamReader(con.getInputStream(), StandardCharsets.UTF_8))) {
                StringBuilder response = new StringBuilder();
                String responseLine;
                while ((responseLine = br.readLine()) != null) {
                    response.append(responseLine.trim());
                }
                
                // Simple JSON parsing to extract the value
                String json = response.toString();
                int rowsIndex = json.indexOf("\"rows\":");
                if (rowsIndex != -1) {
                    int startBracket = json.indexOf("[[", rowsIndex);
                    if (startBracket != -1) {
                        int startQuote = json.indexOf("\"", startBracket);
                        int endQuote = json.indexOf("\"", startQuote + 1);
                        generatedId = json.substring(startQuote + 1, endQuote);
                    }
                }
                if (generatedId.isEmpty()) {
                    System.err.println("Failed to extract ID from response: " + json);
                }
            }

            // 3. Commit Transaction
            URL commitUrl = new URL("http://spanner:9020/v1/" + sessionName + ":commit");
            HttpURLConnection commitCon = (HttpURLConnection) commitUrl.openConnection();
            commitCon.setRequestMethod("POST");
            commitCon.setRequestProperty("Content-Type", "application/json");
            commitCon.setDoOutput(true);

            String commitJson = "{\"transactionId\": \"" + transactionId + "\", \"mutations\": []}";
            try (OutputStream os = commitCon.getOutputStream()) {
                byte[] input = commitJson.getBytes(StandardCharsets.UTF_8);
                os.write(input, 0, input.length);
            }
            commitCon.getResponseCode(); // Execute the request

            return generatedId;
        } catch (Exception e) {
            System.err.println("Error executing SQL: " + e.getMessage());
        }
        return "";
    }
}
