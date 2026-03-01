#ifndef DUAL_BUFFER_H
#define DUAL_BUFFER_H

#include <mysql/mysql.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "../id_generator.h"

struct Segment {
  uint64_t current_id;
  uint64_t max_id;
  uint64_t step;
  bool is_ready;
  Segment() : current_id(1), max_id(0), step(1000), is_ready(false) {}
};

/**
 * Pre-Generated Blocks & Dual Buffering ID Generator
 *
 * Fetches blocks of IDs from a database to minimize DB hits.
 * Uses a background thread to fetch the next block into a secondary buffer
 * before the primary buffer is exhausted, ensuring low latency.
 */
class DualBufferGenerator : public IdGenerator {
 private:
  MYSQL* conn;
  Segment segments[2];
  int current_pos;

  std::mutex mtx;                    // Protects buffer state
  std::mutex db_mtx;                 // Protects DB connection
  std::condition_variable cv_fetch;  // Wakes up background fetcher
  std::condition_variable
      cv_consume;  // Wakes up consumers waiting for new segment

  std::thread fetch_thread;
  std::atomic<bool> is_running;
  std::atomic<bool> fetch_needed;

  void connect();
  bool fetch_segment(int index);
  void background_fetcher();

 public:
  DualBufferGenerator();
  ~DualBufferGenerator();

  uint64_t next_id() override;
};

#endif  // DUAL_BUFFER_H
