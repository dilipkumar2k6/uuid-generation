#ifndef NETWORK_UTIL_H
#define NETWORK_UTIL_H

#include <cstdint>
#include <cstring>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <iostream>
#include "id_generator.h"

/**
 * Derives a Node/Machine/Shard ID from the container's IPv4 address.
 * 
 * This function iterates through the network interfaces, finds the first 
 * non-loopback IPv4 address, and extracts the required number of bits 
 * using the provided bitmask.
 * 
 * @param mask The bitmask to apply to the IP address (e.g., MAX_NODE_ID)
 * @return The derived ID, or 1 if no suitable interface is found.
 */
inline uint64_t get_node_id_from_ip(uint64_t mask = MAX_NODE_ID) {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;
    uint64_t node_id = 1; // Default fallback ID

    if (getifaddrs(&interfaces) == 0) {
        temp_addr = interfaces;
        while (temp_addr != nullptr) {
            if (temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if it's not the loopback interface
                if (strcmp(temp_addr->ifa_name, "lo") != 0) {
                    struct sockaddr_in *pAddr = (struct sockaddr_in *)temp_addr->ifa_addr;
                    uint32_t ip = ntohl(pAddr->sin_addr.s_addr);
                    // Extract the bits of the IP address based on the mask
                    node_id = ip & mask;
                    std::cout << "Derived Node ID " << node_id << " from IP interface " << temp_addr->ifa_name << std::endl;
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

#endif // NETWORK_UTIL_H
