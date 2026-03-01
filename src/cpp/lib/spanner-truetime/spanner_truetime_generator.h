#ifndef SPANNER_TRUETIME_GENERATOR_H
#define SPANNER_TRUETIME_GENERATOR_H

#include <mutex>
#include <string>

#include "../id_generator.h"

class SpannerTrueTimeGenerator : public IdGenerator {
 private:
  std::string spanner_endpoint;
  std::string project_id;
  std::string instance_id;
  std::string database_id;
  std::string session_name;
  std::string shard_id;
  std::mutex mtx;

  std::string http_post(const std::string& url, const std::string& data);
  void create_session();

 public:
  SpannerTrueTimeGenerator();
  ~SpannerTrueTimeGenerator();
  std::string next_id_string() override;
  uint64_t next_id() override { return 0; }  // Not used
};

#endif  // SPANNER_TRUETIME_GENERATOR_H
