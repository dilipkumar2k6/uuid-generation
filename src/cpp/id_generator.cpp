#include "lib/id_generator.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include "lib/db-auto-inc/db_auto_inc.h"
#include "lib/dual-buffer/dual_buffer.h"
#include "lib/etcd-snowflake/etcd_snowflake.h"
#include "lib/hlc-snowflake/hlc_snowflake.h"
#include "lib/insta-snowflake/insta_snowflake.h"
#include "lib/snowflake/snowflake.h"
#include "lib/sonyflake/sonyflake.h"
#include "lib/spanner-truetime/spanner_truetime_generator.h"
#include "lib/spanner/spanner_generator.h"
#include "lib/uuidv4/uuidv4_generator.h"
#include "lib/uuidv7/uuidv7_generator.h"

using namespace std;

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // ---------------------------------------------------------
  // 1. Determine Generator Type
  // ---------------------------------------------------------
  const char* gen_type_env = getenv("GENERATOR_TYPE");
  string gen_type = gen_type_env ? gen_type_env : "SNOWFLAKE";

  unique_ptr<IdGenerator> generator;

  if (gen_type == "HLC_SNOWFLAKE") {
    cout << "Initializing HLC Snowflake generator..." << endl;
    generator = make_unique<HlcSnowflake>();
  } else if (gen_type == "INSTA_SNOWFLAKE") {
    cout << "Initializing Instagram Snowflake generator..." << endl;
    generator = make_unique<InstaSnowflake>();
  } else if (gen_type == "SONYFLAKE") {
    cout << "Initializing Sonyflake generator..." << endl;
    generator = make_unique<Sonyflake>();
  } else if (gen_type == "UUIDV4") {
    cout << "Initializing UUID Version 4 generator..." << endl;
    generator = make_unique<UuidV4Generator>();
  } else if (gen_type == "UUIDV7") {
    cout << "Initializing UUID Version 7 generator..." << endl;
    generator = make_unique<UuidV7Generator>();
  } else if (gen_type == "DB_AUTO_INC") {
    cout << "Initializing Database Auto-Increment generator..." << endl;
    generator = make_unique<DbAutoIncGenerator>();
  } else if (gen_type == "DUAL_BUFFER") {
    cout << "Initializing Dual Buffer generator..." << endl;
    generator = make_unique<DualBufferGenerator>();
  } else if (gen_type == "ETCD_SNOWFLAKE") {
    cout << "Initializing Etcd-Coordinated Snowflake generator..." << endl;
    generator = make_unique<EtcdSnowflake>();
  } else if (gen_type == "SPANNER") {
    cout << "Initializing Spanner Sequence generator..." << endl;
    generator = make_unique<SpannerGenerator>();
  } else if (gen_type == "SPANNER_TRUETIME") {
    cout << "Initializing Spanner TrueTime generator..." << endl;
    generator = make_unique<SpannerTrueTimeGenerator>();
  } else {
    cout << "Initializing Standard Snowflake generator..." << endl;
    generator = make_unique<Snowflake>();
  }

  // ---------------------------------------------------------
  // 2. Setup TCP Server Socket
  // ---------------------------------------------------------
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Allow reuse of address and port to prevent "Address already in use" errors
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt failed");
    exit(EXIT_FAILURE);
  }

  // Configure server address to listen on all interfaces, port 8080
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  // Bind the socket to the address
  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // Start listening for incoming connections (max 3 pending connections)
  if (listen(server_fd, 3) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  cout << "Sidecar listening on port 8080..." << endl;

  // ---------------------------------------------------------
  // 3. Main Server Loop
  // ---------------------------------------------------------
  while (true) {
    // Accept an incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address,
                             (socklen_t*)&addrlen)) < 0) {
      perror("Accept failed");
      continue;
    }

    // Generate a new UUID string and send it to the connected client
    string uuid_str = generator->next_id_string();
    send(new_socket, uuid_str.c_str(), uuid_str.length(), 0);

    // Close the connection immediately after sending (stateless IPC)
    close(new_socket);
  }

  return 0;
}
