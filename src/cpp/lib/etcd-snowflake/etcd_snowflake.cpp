#include "etcd_snowflake.h"

#include <curl/curl.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace std;

// Helper function to write curl response to string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                            void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

EtcdSnowflake::EtcdSnowflake() {
  const char* etcd_host =
      getenv("ETCD_SERVICE_HOST") ? getenv("ETCD_SERVICE_HOST") : "etcd";
  const char* etcd_port =
      getenv("ETCD_SERVICE_PORT") ? getenv("ETCD_SERVICE_PORT") : "2379";
  etcd_endpoint = string("http://") + etcd_host + ":" + etcd_port + "/v3";

  curl_global_init(CURL_GLOBAL_ALL);
  node_id = claim_node_id();

  // Start a background thread to keep the lease alive
  thread([this]() { keep_alive_lease(); }).detach();
}

EtcdSnowflake::~EtcdSnowflake() { curl_global_cleanup(); }

string EtcdSnowflake::http_post(const string& url, const string& data) {
  CURL* curl;
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

uint64_t EtcdSnowflake::claim_node_id() {
  // 1. Create a lease with a 10-second TTL
  string lease_url = etcd_endpoint + "/lease/grant";
  string lease_req = R"({"TTL": 10})";
  string lease_resp = http_post(lease_url, lease_req);

  // Extremely basic JSON parsing to extract the lease ID
  size_t id_pos = lease_resp.find("\"ID\":\"");
  if (id_pos == string::npos) {
    throw runtime_error("Failed to get lease from etcd: " + lease_resp);
  }
  id_pos += 6;
  size_t id_end = lease_resp.find("\"", id_pos);
  lease_id = lease_resp.substr(id_pos, id_end - id_pos);

  cout << "Acquired etcd lease: " << lease_id << endl;

  // 2. Try to claim a Node ID from 0 to 1023
  for (int i = 0; i <= MAX_NODE_ID; ++i) {
    string key = "uuid-generator/node/" + to_string(i);

    // Base64 encode key and value (etcd v3 requires base64)
    // For simplicity in this example, we'll just use a shell command to base64
    // encode In a real production app, use a proper base64 library
    char key_b64[256];
    snprintf(key_b64, sizeof(key_b64), "echo -n '%s' | base64 -w 0",
             key.c_str());
    FILE* pipe = popen(key_b64, "r");
    char buffer[128];
    string encoded_key = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
      encoded_key += buffer;
    }
    pclose(pipe);

    // Remove trailing newline if present
    if (!encoded_key.empty() && encoded_key.back() == '\n') {
      encoded_key.pop_back();
    }

    // Use etcd transaction to only put if the key doesn't exist
    // (create_revision == 0)
    string txn_url = etcd_endpoint + "/kv/txn";
    stringstream txn_req;
    txn_req << R"({
            "compare": [{"target": "CREATE", "key": ")"
            << encoded_key << R"(", "createRevision": 0}],
            "success": [{"requestPut": {"key": ")"
            << encoded_key << R"(", "value": "MQ==", "lease": ")" << lease_id
            << R"("}}]
        })";

    string txn_resp = http_post(txn_url, txn_req.str());

    // Check if the transaction succeeded
    if (txn_resp.find("\"succeeded\":true") != string::npos) {
      cout << "Successfully claimed Node ID: " << i << endl;
      return i;
    }
  }

  throw runtime_error(
      "Failed to claim any Node ID from etcd (all 1024 IDs in use)");
}

void EtcdSnowflake::keep_alive_lease() {
  string keepalive_url = etcd_endpoint + "/lease/keepalive";
  string keepalive_req = R"({"ID": ")" + lease_id + R"("})";

  while (true) {
    // Send keepalive every 3 seconds (for a 10s TTL)
    this_thread::sleep_for(chrono::seconds(3));
    http_post(keepalive_url, keepalive_req);
  }
}

uint64_t EtcdSnowflake::current_time_millis() {
  return chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch())
      .count();
}

uint64_t EtcdSnowflake::wait_for_next_millis(uint64_t last_ts) {
  uint64_t timestamp = current_time_millis();
  while (timestamp <= last_ts) {
    timestamp = current_time_millis();
  }
  return timestamp;
}

uint64_t EtcdSnowflake::next_id() {
  uint64_t timestamp = current_time_millis();
  uint64_t last_ts = last_timestamp.load();

  if (timestamp < last_ts) {
    cerr << "Clock moved backwards. Refusing to generate id." << endl;
    return 0;
  }

  if (timestamp == last_ts) {
    uint64_t seq = (sequence.fetch_add(1) + 1) & MAX_SEQUENCE;
    if (seq == 0) {
      timestamp = wait_for_next_millis(last_ts);
    }
  } else {
    sequence.store(0);
  }

  last_timestamp.store(timestamp);

  uint64_t id = ((timestamp - EPOCH) << TIMESTAMP_SHIFT) |
                (node_id << NODE_ID_SHIFT) | sequence.load();

  return id;
}
