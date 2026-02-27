#include "spanner_generator.h"
#include <iostream>
#include <stdexcept>
#include <curl/curl.h>
#include <sstream>
#include <cstdlib>

using namespace std;

// Helper function to write curl response to string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

SpannerGenerator::SpannerGenerator() {
    const char* spanner_host = getenv("SPANNER_EMULATOR_HOST") ? getenv("SPANNER_EMULATOR_HOST") : "spanner:9020";
    project_id = getenv("SPANNER_PROJECT_ID") ? getenv("SPANNER_PROJECT_ID") : "test-project";
    instance_id = getenv("SPANNER_INSTANCE_ID") ? getenv("SPANNER_INSTANCE_ID") : "test-instance";
    database_id = getenv("SPANNER_DATABASE_ID") ? getenv("SPANNER_DATABASE_ID") : "test-db";
    
    spanner_endpoint = string("http://") + spanner_host + "/v1";

    curl_global_init(CURL_GLOBAL_ALL);
    create_session();
}

SpannerGenerator::~SpannerGenerator() {
    // Ideally, we should delete the session here, but for simplicity in this example we'll skip it
    curl_global_cleanup();
}

string SpannerGenerator::http_post(const string& url, const string& data) {
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        // Set timeout to 5 seconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

void SpannerGenerator::create_session() {
    string session_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions";
    string session_req = "{}"; // Empty body for session creation
    
    string session_resp = http_post(session_url, session_req);
    
    // Extremely basic JSON parsing to extract the session name
    size_t name_pos = session_resp.find("\"name\"");
    if (name_pos == string::npos) {
        throw runtime_error("Failed to create session in Spanner: " + session_resp);
    }
    
    // Find the colon after "name"
    size_t colon_pos = session_resp.find(":", name_pos);
    if (colon_pos == string::npos) {
        throw runtime_error("Failed to parse session name (no colon): " + session_resp);
    }
    
    // Find the quote starting the value
    size_t start_quote = session_resp.find("\"", colon_pos);
    if (start_quote == string::npos) {
        throw runtime_error("Failed to parse session name (no start quote): " + session_resp);
    }
    
    // Find the quote ending the value
    size_t end_quote = session_resp.find("\"", start_quote + 1);
    if (end_quote == string::npos) {
        throw runtime_error("Failed to parse session name (no end quote): " + session_resp);
    }
    
    session_name = session_resp.substr(start_quote + 1, end_quote - start_quote - 1);
    
    // Extract just the session ID part if it's a full path
    size_t last_slash = session_name.find_last_of('/');
    if (last_slash != string::npos) {
        session_name = session_name.substr(last_slash + 1);
    }
    
    cout << "Created Spanner session: " << session_name << endl;
}

uint64_t SpannerGenerator::next_id() {
    lock_guard<mutex> lock(mtx);
    
    string query_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions/" + session_name + ":executeSql";
    string query_req = "{\"sql\": \"SELECT GET_NEXT_SEQUENCE_VALUE(SEQUENCE uuid_sequence)\", \"transaction\": {\"begin\": {\"readWrite\": {}}}}";
    
    string query_resp = http_post(query_url, query_req);
    
    // Extract transaction ID
    size_t txn_pos = query_resp.find("\"id\":\"");
    string txn_id = "";
    if (txn_pos != string::npos) {
        txn_pos += 6;
        size_t txn_end = query_resp.find("\"", txn_pos);
        if (txn_end != string::npos) {
            txn_id = query_resp.substr(txn_pos, txn_end - txn_pos);
        }
    }
    
    // Extremely basic JSON parsing to extract the sequence value
    // Expected response format: {"metadata": {...}, "rows": [["1234567890"]]}
    size_t rows_pos = query_resp.find("\"rows\"");
    if (rows_pos == string::npos) {
        cerr << "Failed to execute query in Spanner: " << query_resp << endl;
        return 0;
    }
    
    // Find the start of the value: [["
    size_t val_pos = query_resp.find("[\"", rows_pos);
    if (val_pos == string::npos) {
        cerr << "Failed to parse query response: " << query_resp << endl;
        return 0;
    }
    val_pos += 2; // Move past ["
    
    // Find the end of the value: "]
    size_t val_end = query_resp.find("\"]", val_pos);
    if (val_end == string::npos) {
        cerr << "Failed to parse query response end: " << query_resp << endl;
        return 0;
    }
    
    string val_str = query_resp.substr(val_pos, val_end - val_pos);
    
    // Commit the transaction if we got an ID
    if (!txn_id.empty()) {
        string commit_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions/" + session_name + ":commit";
        string commit_req = "{\"transactionId\": \"" + txn_id + "\"}";
        http_post(commit_url, commit_req);
    }
    
    try {
        return stoull(val_str);
    } catch (const exception& e) {
        cerr << "Failed to convert sequence value to uint64_t: " << val_str << endl;
        return 0;
    }
}
