#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <atomic>
#include <thread>
#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Snowflake parameters
const uint64_t EPOCH = 1609459200000ULL; // Jan 1, 2021
const uint64_t NODE_ID_BITS = 10;
const uint64_t SEQUENCE_BITS = 12;

using namespace std;

const uint64_t MAX_NODE_ID = (static_cast<uint64_t>(1) << NODE_ID_BITS) - 1;
const uint64_t MAX_SEQUENCE = (static_cast<uint64_t>(1) << SEQUENCE_BITS) - 1;

const uint64_t NODE_ID_SHIFT = SEQUENCE_BITS;
const uint64_t TIMESTAMP_SHIFT = SEQUENCE_BITS + NODE_ID_BITS;

class Snowflake {
private:
    uint64_t node_id;
    atomic<uint64_t> sequence{0};
    atomic<uint64_t> last_timestamp{0};

    // Helper to get current time in milliseconds since epoch
    uint64_t current_time_millis() {
        return chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    // Spin-wait until the next millisecond arrives
    uint64_t wait_for_next_millis(uint64_t last_ts) {
        uint64_t timestamp = current_time_millis();
        while (timestamp <= last_ts) {
            timestamp = current_time_millis();
        }
        return timestamp;
    }

public:
    Snowflake(uint64_t node_id) : node_id(node_id & MAX_NODE_ID) {}

    // Generates the next unique 64-bit ID
    uint64_t next_id() {
        uint64_t timestamp = current_time_millis();
        uint64_t last_ts = last_timestamp.load();

        // Handle clock moving backwards
        if (timestamp < last_ts) {
            cerr << "Clock moved backwards. Refusing to generate id." << endl;
            return 0;
        }

        // If in the same millisecond, increment sequence
        if (timestamp == last_ts) {
            uint64_t seq = (sequence.fetch_add(1) + 1) & MAX_SEQUENCE;
            // If sequence overflows, wait for next millisecond
            if (seq == 0) {
                timestamp = wait_for_next_millis(last_ts);
            }
        } else {
            // Reset sequence for new millisecond
            sequence.store(0);
        }

        last_timestamp.store(timestamp);

        // Pack the timestamp, node ID, and sequence into a 64-bit integer
        uint64_t id = ((timestamp - EPOCH) << TIMESTAMP_SHIFT) |
                      (node_id << NODE_ID_SHIFT) |
                      sequence.load();
        
        // Bit reversal technique as requested (flips all bits)
        return ~id;
    }
};

// Helper function to get the last 10 bits of the first non-loopback IPv4 address
uint64_t get_node_id_from_ip() {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;
    uint64_t node_id = 1; // Default fallback

    if (getifaddrs(&interfaces) == 0) {
        temp_addr = interfaces;
        while (temp_addr != nullptr) {
            if (temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if it's not the loopback interface
                if (strcmp(temp_addr->ifa_name, "lo") != 0) {
                    struct sockaddr_in *pAddr = (struct sockaddr_in *)temp_addr->ifa_addr;
                    uint32_t ip = ntohl(pAddr->sin_addr.s_addr);
                    // Extract the last 10 bits of the IP address
                    node_id = ip & MAX_NODE_ID;
                    cout << "Derived Node ID " << node_id << " from IP interface " << temp_addr->ifa_name << endl;
                    break;
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    
    if (interfaces != nullptr) {
        freeifaddrs(interfaces);
    }
    
    return node_id;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Derive Node ID from IP address
    uint64_t node_id = get_node_id_from_ip();
    
    // Initialize Snowflake generator with derived Node ID
    Snowflake generator(node_id);

    // Create a TCP socket for the sidecar server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Allow reuse of address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address to listen on all interfaces, port 8080
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    cout << "Snowflake sidecar listening on port 8080..." << endl;
    
    // Main server loop: accept connections and send UUIDs
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        // Generate a new UUID and send it to the connected client
        uint64_t uuid = generator.next_id();
        send(new_socket, &uuid, sizeof(uuid), 0);
        
        // Close the connection after sending
        close(new_socket);
    }
    
    return 0;
}
