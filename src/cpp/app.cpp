#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

// Mutex to prevent interleaved console output from multiple threads
mutex cout_mutex;

void request_uuid(int thread_id) {
  // Continuously request UUIDs from the Snowflake sidecar
  while (true) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // 1. Create a TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      lock_guard<mutex> lock(cout_mutex);
      cerr << "[Thread " << thread_id << "] Socket creation error" << endl;
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    // 2. Configure the server address (localhost:8080 for sidecar IPC)
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // 3. Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
      lock_guard<mutex> lock(cout_mutex);
      cerr << "[Thread " << thread_id
           << "] Invalid address / Address not supported" << endl;
      close(sock);
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    // 4. Attempt to connect to the sidecar
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      lock_guard<mutex> lock(cout_mutex);
      cerr << "[Thread " << thread_id << "] Connection Failed. Retrying..."
           << endl;
      close(sock);
      this_thread::sleep_for(chrono::seconds(1));
      continue;
    }

    // 5. Read the UUID string from the socket
    char buffer[128] = {0};
    int valread = read(sock, buffer, sizeof(buffer) - 1);

    {
      lock_guard<mutex> lock(cout_mutex);
      if (valread > 0) {
        cout << "[Thread " << thread_id << "] Received UUID: " << buffer
             << endl;
      } else {
        cerr << "[Thread " << thread_id << "] Failed to read UUID" << endl;
      }
    }

    // 6. Close the socket and wait before the next request
    close(sock);
    this_thread::sleep_for(chrono::milliseconds(500));  // Request every 500ms
  }
}

int main() {
  cout << "App container starting with 5 concurrent threads..." << endl;

  const int NUM_THREADS = 5;
  vector<thread> threads;

  // Spawn multiple threads to simulate concurrent requests
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back(request_uuid, i + 1);
  }

  // Join threads (will run indefinitely)
  for (auto& t : threads) {
    t.join();
  }

  return 0;
}
