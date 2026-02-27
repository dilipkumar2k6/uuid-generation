# C++ Concepts in the UUID Generator Codebase

This document explains the high-level C++ concepts used across the entire UUID Generator codebase (including Spanner, Snowflake, UUIDv4, Dual Buffer, and the App container). Each section covers the basics, provides a minimal working example, and explains how it is used to generate UUIDs.

## 1. Object-Oriented Programming (Classes & Inheritance)
**Basics:** 
Classes bundle data and functions together. Inheritance allows a new class to inherit behaviors from an existing class, enabling code reuse and a unified interface (polymorphism).

**What is a Virtual Function?**
A `virtual` function is a function in a base class that you expect to redefine (override) in a derived class. It enables **polymorphism**—meaning if you have a generic pointer to a base class (like `Animal*`), but it actually points to a specific derived class (like `Dog`), calling the virtual function will execute the `Dog`'s version.
A *pure* virtual function (written with `= 0` at the end) means the base class provides no implementation at all. It forces any derived class to implement that function, effectively turning the base class into an "Interface".

**Minimal Example:**
```cpp
class Animal {
public:
    virtual void speak() = 0; // Pure virtual function (Interface)
};

class Dog : public Animal {
public:
    void speak() override { std::cout << "Woof!\n"; } // Overridden function
};
```

**Usage in UUID Generation:**
The project defines a base `IdGenerator` class (`id_generator.h`) with a pure virtual `next_id()` method. Specific generators like `SpannerGenerator`, `Snowflake`, and `UuidV4Generator` inherit from this base class and *override* `next_id()`. This allows the main server to use a generic `IdGenerator*` pointer to call `next_id()` without needing to know exactly which algorithm is running under the hood.

## 2. Header (`.h`) vs Implementation (`.cpp`) Files
**Basics:**
In C++, code is typically split into two types of files:
- **Header Files (`.h` or `.hpp`)**: These contain *declarations*. They tell the compiler "what" exists (classes, function names, variables) without providing the actual logic. They act as a menu or an interface.
- **Implementation Files (`.cpp`)**: These contain *definitions*. They provide the actual logic (the "how") for the things declared in the header file.

When you want to use a class in another file, you `#include` its header file, not its `.cpp` file.

**Minimal Example:**
`math_utils.h` (Declaration)
```cpp
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

class MathUtils {
public:
    int add(int a, int b); // Just the signature
};

#endif
```

`math_utils.cpp` (Implementation)
```cpp
#include "math_utils.h"

// The actual logic
int MathUtils::add(int a, int b) {
    return a + b;
}
```

**Usage in UUID Generation:**
Every generator in the project follows this pattern. For example, `snowflake.h` declares the `Snowflake` class, its private variables (`node_id`, `sequence`), and its public methods (`next_id()`). The `snowflake.cpp` file then `#include "snowflake.h"` and writes the actual C++ code to generate the IDs. 

This separation keeps the code organized, speeds up compilation (because if you change the logic in `.cpp`, files that `#include` the `.h` don't need to be recompiled), and hides internal implementation details from the rest of the program.

## 3. Multithreading (`std::thread`)
**Basics:**
Multithreading allows a program to run multiple tasks concurrently (at the same time), making better use of multi-core processors.

**Minimal Example:**
```cpp
#include <thread>
#include <iostream>

void task_a() { std::cout << "Task A is running...\n"; }
void task_b() { std::cout << "Task B is running...\n"; }

int main() {
    std::thread t1(task_a); // Starts Task A in a new thread
    std::thread t2(task_b); // Starts Task B in another new thread
    
    // The main program continues here while t1 and t2 run simultaneously
    
    t1.join(); // Waits for Task A to finish
    t2.join(); // Waits for Task B to finish
}
```

**Usage in UUID Generation:**
In `app.cpp`, multiple threads are spawned to simulate concurrent clients requesting UUIDs simultaneously. In `dual_buffer.cpp`, a background thread is created to fetch the next batch of IDs from the MySQL database while the main thread continues serving IDs to clients without pausing.

## 4. Synchronization (`std::mutex`, `std::condition_variable`, `std::shared_mutex`)
**Basics:**
When multiple threads access shared data, it can cause race conditions. 
- A `std::mutex` acts as an exclusive lock to ensure only one thread accesses the data at a time. 
- A `std::condition_variable` allows threads to sleep and wait for a specific condition to become true (like waiting for data to be ready), avoiding busy-waiting (spinning in a loop).
- A `std::shared_mutex` (Reader-Writer Lock) allows *multiple* threads to read data simultaneously, but requires *exclusive* access to write data.

**Minimal Example (Condition Variable):**
```cpp
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

std::mutex mtx;
std::condition_variable cv;
bool data_ready = false;

void worker_thread() {
    std::unique_lock<std::mutex> lock(mtx);
    // Sleep until data_ready becomes true
    cv.wait(lock, []{ return data_ready; }); 
    std::cout << "Worker: Data is ready, processing...\n";
}

int main() {
    std::thread t(worker_thread);
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        data_ready = true; // Prepare the data
    }
    cv.notify_one(); // Wake up the sleeping worker thread
    
    t.join();
}
```

**Minimal Example (Reader-Writer Lock):**
```cpp
#include <shared_mutex>
#include <iostream>

std::shared_mutex rw_mtx;
int shared_data = 0;

void reader() {
    // Multiple readers can hold this lock at the same time
    std::shared_lock<std::shared_mutex> lock(rw_mtx); 
    std::cout << "Read: " << shared_data << "\n";
}

void writer() {
    // Only ONE writer can hold this lock. It blocks all readers and other writers.
    std::unique_lock<std::shared_mutex> lock(rw_mtx); 
    shared_data++;
    std::cout << "Wrote: " << shared_data << "\n";
}
```

**Usage in UUID Generation:**
Mutexes are used everywhere: to protect the random number generator in `uuidv4_generator.cpp`, to protect the MySQL connection in `dual_buffer.cpp`, and to prevent interleaved console output in `app.cpp`. 

`dual_buffer.cpp` heavily relies on `std::condition_variable`. The background fetcher thread uses `cv.wait()` to sleep until the main thread signals that the current ID buffer is running low. Once the background thread fetches new IDs from MySQL, it uses `cv.notify_all()` to wake up any client threads that were waiting for the new IDs to arrive.

## 5. Atomic Operations (`std::atomic`)
**Basics:**
Atomic variables provide a lock-free way to safely read and modify data across multiple threads. They are faster than mutexes for simple operations like incrementing a counter.

**Minimal Example:**
```cpp
#include <atomic>
std::atomic<int> counter(0);

void increment() {
    counter.fetch_add(1); // Thread-safe lock-free increment
}
```

**Usage in UUID Generation:**
In `snowflake.cpp`, `std::atomic<uint64_t> sequence` is used to safely increment the sequence number when multiple IDs are requested within the exact same millisecond, avoiding the overhead of a full mutex lock.

## 6. Time Management (`std::chrono`)
**Basics:**
The `<chrono>` library provides precision time utilities, allowing you to measure durations, get the current system time, and pause threads.

**Minimal Example:**
```cpp
#include <chrono>
#include <iostream>

auto now = std::chrono::system_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
std::cout << "Milliseconds since 1970: " << ms << "\n";
```

**Usage in UUID Generation:**
Time is the core component of the Snowflake algorithm. `snowflake.cpp` uses `std::chrono` to get the current timestamp in milliseconds since the UNIX epoch. This timestamp forms the first 41 bits of the generated 64-bit ID, ensuring IDs are sortable by time.

## 7. Bitwise Operations (`<<`, `|`, `&`)
**Basics:**
Bitwise operators manipulate data at the binary level (1s and 0s). Shift (`<<`) moves bits to the left. OR (`|`) combines bits. AND (`&`) masks bits.

**Minimal Example:**
```cpp
uint64_t part1 = 1; // Binary: 0001
uint64_t part2 = 2; // Binary: 0010
uint64_t combined = (part1 << 4) | part2; // Binary: 00010010 (18 in decimal)
```

**Usage in UUID Generation:**
In `snowflake.cpp`, bitwise operations combine three separate numbers (timestamp, node ID, and sequence) into a single 64-bit integer. In `uuidv4_generator.cpp`, bitwise AND and OR are used to force specific bits to match the RFC 4122 standard (setting the version to `4` and the variant to `10xx`).

## 8. Random Number Generation (`<random>`)
**Basics:**
The `<random>` library provides modern, high-quality random number generators, replacing the older, less secure `rand()` function.

**Minimal Example:**
```cpp
#include <random>

std::random_device rd; // Hardware entropy source
std::mt19937_64 gen(rd()); // 64-bit Mersenne Twister engine
std::uniform_int_distribution<uint64_t> dis;

uint64_t random_num = dis(gen);
```

**Usage in UUID Generation:**
`uuidv4_generator.cpp` uses the 64-bit Mersenne Twister (`std::mt19937_64`) to generate 128 bits of highly random data. This randomness forms the entirely of the UUIDv4 string (except for the version and variant bits).

## 9. Networking (Sockets & libcurl)
**Basics:**
Sockets are the fundamental way programs communicate over a network. `libcurl` is a popular library that simplifies making HTTP requests (like GET and POST).

**Minimal Example:**
```cpp
// Basic socket creation (POSIX)
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
read(sock, buffer, 1024);
```

**Usage in UUID Generation:**
`app.cpp` uses raw POSIX TCP sockets (`<sys/socket.h>`) to connect to the local sidecar container on port 8080 and read the generated UUID strings. `spanner_generator.cpp` uses `libcurl` to send HTTP POST requests to the Google Cloud Spanner emulator to execute SQL queries.

## 10. String Formatting and Conversion
**Basics:**
C++ provides tools to convert strings to numbers (`std::stoull`), numbers to strings, and to format output nicely (`std::stringstream`, `std::hex`).

**Minimal Example:**
```cpp
#include <string>
#include <sstream>
#include <iomanip>

uint64_t num = std::stoull("12345"); // String to integer
std::stringstream ss;
ss << std::hex << std::setfill('0') << std::setw(8) << 255; // Formats as "000000ff"
```

**Usage in UUID Generation:**
`std::stoull` is used heavily to convert text responses from databases (MySQL, Spanner) into 64-bit integers. `std::stringstream` and `std::hex` are used in `uuidv4_generator.cpp` to format the raw 128-bit integer data into the standard `8-4-4-4-12` hexadecimal UUID string format.

## 11. Exception Handling (`try`, `catch`, `throw`)
**Basics:**
Exceptions handle runtime errors gracefully without crashing the program. `throw` triggers an error, and `try-catch` blocks handle it.

**Minimal Example:**
```cpp
try {
    throw std::runtime_error("Database connection failed!");
} catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << "\n";
}
```

**Usage in UUID Generation:**
Exceptions are used to handle critical failures, such as when `dual_buffer.cpp` fails to initialize the MySQL connection, or when `spanner_generator.cpp` fails to parse the JSON response from the Spanner emulator. It allows the program to log the error and recover or exit safely.

## 12. Size Types (`size_t`)
**Basics:**
`size_t` is an unsigned integer type used to represent the size of objects in bytes or the number of elements in an array or string. It is guaranteed to be large enough to contain the size of the biggest object the host system can handle (e.g., 64 bits on a 64-bit system). Because it is unsigned, it can never be negative.

**Minimal Example:**
```cpp
#include <string>
#include <iostream>
#include <vector>

std::string text = "Hello World";
size_t length = text.length(); // Returns 11

std::vector<int> numbers = {1, 2, 3, 4, 5};
for (size_t i = 0; i < numbers.size(); ++i) {
    std::cout << numbers[i] << " ";
}
```

**Usage in UUID Generation:**
`size_t` is used extensively throughout the codebase whenever dealing with memory sizes, string lengths, or array indices. For example:
- In `spanner_generator.cpp`, `size_t` is used to store the positions returned by `std::string::find()` when parsing JSON responses (e.g., `size_t name_pos = session_resp.find("\"name\"");`). If the substring is not found, `find()` returns a special constant `std::string::npos`, which is also of type `size_t`.
- In the `WriteCallback` function for `libcurl`, `size_t` is used to represent the size of the incoming data chunks (`size_t size, size_t nmemb`).

## 13. Smart Pointers (`std::unique_ptr`)
**Basics:**
In older C++, developers had to manually allocate memory with `new` and free it with `delete`. If they forgot to call `delete`, it caused a "memory leak". 
A `std::unique_ptr` is a "smart pointer" that automatically frees the memory it points to when the pointer goes out of scope (e.g., when a function ends or an object is destroyed). The word "unique" means that only *one* `unique_ptr` can own a specific piece of memory at a time.

**Minimal Example:**
```cpp
#include <memory>
#include <iostream>

class MyClass {
public:
    MyClass() { std::cout << "Created\n"; }
    ~MyClass() { std::cout << "Destroyed\n"; }
    void do_work() { std::cout << "Working...\n"; }
};

void example() {
    // Automatically allocates memory
    std::unique_ptr<MyClass> ptr = std::make_unique<MyClass>();
    ptr->do_work();
    
    // When the function ends, 'ptr' goes out of scope.
    // The memory is automatically freed and ~MyClass() is called.
    // No need to call 'delete'!
}
```

**Usage in UUID Generation:**
In `id_generator.cpp`, the `IdGenerator::create()` factory function returns a `std::unique_ptr<IdGenerator>`. Depending on the configuration, it creates a specific generator (like `new SpannerGenerator()` or `new Snowflake()`) and wraps it in a `unique_ptr`. 

This guarantees that whoever calls `create()` will safely own the generator. When the main server shuts down and the `unique_ptr` is destroyed, C++ will automatically call the correct destructor (e.g., `~SpannerGenerator()`) to clean up resources like database connections or libcurl sessions, preventing memory leaks.

## 14. Fixed-Width Integer Types (`uint64_t`)
**Basics:**
In C++, the size of standard integer types like `int` or `long` can vary depending on the operating system and compiler (e.g., an `int` might be 16 bits on an old system, but 32 bits on a modern one). 
When you need an integer to be an *exact* size, you use fixed-width integer types from the `<cstdint>` library. `uint64_t` stands for **U**nsigned **INT**eger **64**-bi**T**. 
- "Unsigned" means it cannot hold negative numbers.
- "64-bit" means it is exactly 8 bytes large, capable of holding numbers from 0 up to 18,446,744,073,709,551,615.

**Minimal Example:**
```cpp
#include <cstdint>
#include <iostream>

uint64_t large_number = 18446744073709551615ULL; // Max 64-bit value
// The 'ULL' suffix tells the compiler this is an Unsigned Long Long literal

std::cout << large_number << "\n";
```

**Usage in UUID Generation:**
`uint64_t` is the absolute core data type of this entire project. The base `IdGenerator` interface defines `virtual uint64_t next_id() = 0;`. 
Every single ID generator (Snowflake, Spanner, Dual Buffer, etc.) must return a `uint64_t`. This guarantees that the generated IDs are exactly 64 bits long, which is a common requirement for database primary keys (often mapped to `BIGINT UNSIGNED` in SQL) to ensure they are large enough to never run out, but small enough to be efficient for indexing.

## 15. HTTP Requests with libcurl (`curl_easy_setopt`)
**Basics:**
`libcurl` is a popular C library for transferring data over network protocols like HTTP. The "easy" interface of libcurl is designed to be simple and synchronous (blocking). 
The core function used to configure a libcurl request is `curl_easy_setopt`. It takes three arguments:
1. The curl handle (the object representing the request).
2. An option to set (e.g., `CURLOPT_URL` for the web address).
3. The value for that option.

**Minimal Example:**
```cpp
#include <curl/curl.h>
#include <iostream>

void fetch_website() {
    CURL *curl = curl_easy_init(); // Initialize the handle
    if (curl) {
        // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, "http://example.com");
        
        // Perform the request (this blocks until it finishes)
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "Request failed: " << curl_easy_strerror(res) << "\n";
        }
        
        curl_easy_cleanup(curl); // Clean up the handle
    }
}
```

**Usage in UUID Generation:**
In `spanner_generator.cpp`, the `http_post` function uses `curl_easy_setopt` extensively to configure the HTTP requests sent to the Google Cloud Spanner emulator. 
It sets:
- `CURLOPT_URL`: The specific Spanner API endpoint (e.g., to create a session or execute SQL).
- `CURLOPT_POSTFIELDS`: The JSON body containing the SQL query.
- `CURLOPT_WRITEFUNCTION` and `CURLOPT_WRITEDATA`: These tell libcurl to pass any incoming response data to a custom C++ callback function (`WriteCallback`), which appends the data into a `std::string` so the program can parse the JSON response later.
- `CURLOPT_TIMEOUT`: Ensures the program doesn't hang forever if the Spanner emulator is unresponsive.

## 16. String to Integer Conversion (`std::stoull`)
**Basics:**
When you receive data from a network request or a database, it often comes back as text (a `std::string`). If that text represents a number, you need to convert it into an actual integer type before you can do math with it or return it.
`std::stoull` stands for **S**tring **TO** **U**nsigned **L**ong **L**ong. It takes a `std::string` and converts it into a 64-bit unsigned integer (`uint64_t`).

**Minimal Example:**
```cpp
#include <string>
#include <iostream>

std::string number_str = "1234567890";

// Convert the string into a 64-bit integer
uint64_t actual_number = std::stoull(number_str);

// Now we can do math with it
std::cout << actual_number + 10 << "\n"; // Outputs 1234567900
```

**Usage in UUID Generation:**
In `spanner_generator.cpp`, when the Spanner emulator responds to the SQL query `SELECT GET_NEXT_SEQUENCE_VALUE(...)`, it returns a JSON string that looks something like this: `{"rows": [["1234567890"]]}`. 

The code extracts the `"1234567890"` part into a `std::string` called `val_str`. However, the `next_id()` function is required to return a `uint64_t`, not a string. Therefore, `std::stoull(val_str)` is used to convert that text into the final 64-bit numerical ID. This is wrapped in a `try-catch` block because `stoull` will throw an exception if the string doesn't actually contain a valid number.

## 17. Network Interfaces (`getifaddrs` and `struct ifaddrs`)
**Basics:**
When a program needs to know the IP address of the machine it is running on, it can use the POSIX C function `getifaddrs()`. This function asks the operating system for a list of all active network interfaces (like Wi-Fi, Ethernet, or the local loopback `lo`).
It returns this list as a "linked list" of `struct ifaddrs`. A linked list is a data structure where each item contains a pointer (`ifa_next`) to the next item in the list. You iterate through the list until the pointer is `nullptr`.

**Minimal Example:**
```cpp
#include <ifaddrs.h>
#include <iostream>

void print_interfaces() {
    struct ifaddrs *interfaces = nullptr;
    
    // Get the list of interfaces from the OS
    if (getifaddrs(&interfaces) == 0) {
        struct ifaddrs *temp = interfaces;
        
        // Loop through the linked list
        while (temp != nullptr) {
            std::cout << "Found interface: " << temp->ifa_name << "\n";
            temp = temp->ifa_next; // Move to the next item
        }
        
        // Free the memory allocated by the OS
        freeifaddrs(interfaces);
    }
}
```

**Usage in UUID Generation:**
In `network_util.h`, the Snowflake algorithms need a unique `node_id` (or machine ID) so that if multiple containers are generating IDs at the exact same millisecond, they don't generate the exact same ID. 

Instead of requiring the user to manually configure a `node_id` for every container, the `get_node_id_from_ip()` function uses `getifaddrs()` to automatically find the container's IP address. It loops through the `ifaddrs` linked list, ignores the local loopback (`"lo"`), extracts the IPv4 address, and uses the last few bits of that IP address as the unique `node_id`.
