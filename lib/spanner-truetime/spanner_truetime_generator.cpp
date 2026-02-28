#include "spanner_truetime_generator.h"
#include <iostream>
#include <stdexcept>
#include <curl/curl.h>
#include <sstream>
#include <cstdlib>
#include <random>
#include <iomanip>

using namespace std;

// Helper function to write curl response to string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

SpannerTrueTimeGenerator::SpannerTrueTimeGenerator() {
    const char* spanner_host = getenv("SPANNER_EMULATOR_HOST") ? getenv("SPANNER_EMULATOR_HOST") : "spanner:9020";
    project_id = getenv("SPANNER_PROJECT_ID") ? getenv("SPANNER_PROJECT_ID") : "test-project";
    instance_id = getenv("SPANNER_INSTANCE_ID") ? getenv("SPANNER_INSTANCE_ID") : "test-instance";
    database_id = getenv("SPANNER_DATABASE_ID") ? getenv("SPANNER_DATABASE_ID") : "test-db";
    
    spanner_endpoint = string("http://") + spanner_host + "/v1";

    // Generate a random 4-character hex string for the Shard ID
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 65535);
    stringstream ss;
    ss << hex << setfill('0') << setw(4) << dis(gen);
    shard_id = ss.str();

    curl_global_init(CURL_GLOBAL_ALL);
    create_session();
}

SpannerTrueTimeGenerator::~SpannerTrueTimeGenerator() {
    curl_global_cleanup();
}

string SpannerTrueTimeGenerator::http_post(const string& url, const string& data) {
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

void SpannerTrueTimeGenerator::create_session() {
    string session_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions";
    string session_req = "{}";
    
    string session_resp = http_post(session_url, session_req);
    
    size_t name_pos = session_resp.find("\"name\"");
    if (name_pos == string::npos) {
        throw runtime_error("Failed to create session in Spanner: " + session_resp);
    }
    
    size_t colon_pos = session_resp.find(":", name_pos);
    size_t start_quote = session_resp.find("\"", colon_pos);
    size_t end_quote = session_resp.find("\"", start_quote + 1);
    
    session_name = session_resp.substr(start_quote + 1, end_quote - start_quote - 1);
    
    size_t last_slash = session_name.find_last_of('/');
    if (last_slash != string::npos) {
        session_name = session_name.substr(last_slash + 1);
    }
    
    cout << "Created Spanner session: " << session_name << " with Shard ID: " << shard_id << endl;
}

string SpannerTrueTimeGenerator::next_id_string() {
    lock_guard<mutex> lock(mtx);
    
    // 1. Begin Transaction
    string begin_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions/" + session_name + ":beginTransaction";
    string begin_req = "{\"options\": {\"readWrite\": {}}}";
    
    string begin_resp = http_post(begin_url, begin_req);
    
    size_t id_pos = begin_resp.find("\"id\"");
    if (id_pos == string::npos) {
        cerr << "Failed to begin transaction: " << begin_resp << endl;
        return "";
    }
    
    size_t colon_pos = begin_resp.find(":", id_pos);
    size_t start_quote = begin_resp.find("\"", colon_pos);
    size_t end_quote = begin_resp.find("\"", start_quote + 1);
    string txn_id = begin_resp.substr(start_quote + 1, end_quote - start_quote - 1);
    
    // 2. Commit Transaction
    string commit_url = spanner_endpoint + "/projects/" + project_id + "/instances/" + instance_id + "/databases/" + database_id + "/sessions/" + session_name + ":commit";
    string commit_req = "{\"transactionId\": \"" + txn_id + "\", \"mutations\": []}";
    
    string commit_resp = http_post(commit_url, commit_req);
    
    size_t ts_pos = commit_resp.find("\"commitTimestamp\"");
    if (ts_pos == string::npos) {
        cerr << "Failed to commit transaction: " << commit_resp << endl;
        return "";
    }
    
    colon_pos = commit_resp.find(":", ts_pos);
    start_quote = commit_resp.find("\"", colon_pos);
    end_quote = commit_resp.find("\"", start_quote + 1);
    string commit_ts = commit_resp.substr(start_quote + 1, end_quote - start_quote - 1);
    
    // Format: ShardID-CommitTimestamp-TransactionID
    // Transaction ID is base64 encoded and can be long, so we take the first 8 characters for brevity in the UUID
    string short_txn_id = txn_id.length() > 8 ? txn_id.substr(0, 8) : txn_id;
    
    return shard_id + "-" + commit_ts + "-" + short_txn_id;
}
